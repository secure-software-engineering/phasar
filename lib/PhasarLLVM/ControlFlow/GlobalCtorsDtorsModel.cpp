/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/GlobalCtorsDtorsModel.h"

#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"

#include <concepts>
#include <functional>
#include <map>

using namespace psr;

namespace {

constexpr llvm::StringLiteral GlobalCtorsName = "llvm.global_ctors";
constexpr llvm::StringLiteral GlobalDtorsName = "llvm.global_dtors";

void forEachGlobalCtorDtor(
    const llvm::Module &M, llvm::StringRef GlobalName,
    std::invocable<size_t, llvm::Function *> auto Handler) {
  const auto *Gtors = M.getGlobalVariable(GlobalName);
  if (Gtors == nullptr) {
    return;
  }

  if (!llvm::isa<llvm::ArrayType>(Gtors->getValueType())) {
    return;
  }

  const auto *ConstFunArray =
      llvm::dyn_cast<llvm::ConstantArray>(Gtors->getInitializer());
  if (!ConstFunArray) {
    return;
  }

  for (const auto &Op : ConstFunArray->operands()) {
    const auto *FunDesc = llvm::dyn_cast<llvm::ConstantStruct>(Op);
    if (!FunDesc) {
      continue;
    }

    auto *Fun = llvm::dyn_cast<llvm::Function>(FunDesc->getOperand(1));
    const auto *Prio =
        llvm::dyn_cast<llvm::ConstantInt>(FunDesc->getOperand(0));
    if (Fun && Prio) {
      Handler(size_t(Prio->getLimitedValue(SIZE_MAX)), Fun);
    }
  }
}

[[nodiscard]] auto collectGlobalCtors(const llvm::Module &Mod) {
  std::multimap<size_t, llvm::Function *> Ret;
  forEachGlobalCtorDtor(
      Mod, GlobalCtorsName,
      [&Ret](size_t Prio, llvm::Function *Fun) { Ret.emplace(Prio, Fun); });
  return Ret;
}

[[nodiscard]] auto collectGlobalDtors(const llvm::Module &Mod) {
  std::multimap<size_t, llvm::Function *, std::greater<>> Ret;
  forEachGlobalCtorDtor(
      Mod, GlobalDtorsName,
      [&Ret](size_t Prio, llvm::Function *Fun) { Ret.emplace(Prio, Fun); });
  return Ret;
}

[[nodiscard]] auto collectRegisteredDtorsAtExit(const llvm::Module &Mod) {
  llvm::SmallVector<llvm::FunctionCallee> Ret;

  auto *AtExitFn = Mod.getFunction("atexit");
  if (!AtExitFn) {
    return Ret;
  }

  for (auto *User : AtExitFn->users()) {
    auto *Call = llvm::dyn_cast<llvm::CallBase>(User);
    if (!Call) {
      continue;
    }

    auto *DtorOp = llvm::dyn_cast_or_null<llvm::Function>(
        Call->getArgOperand(0)->stripPointerCastsAndAliases());
    if (!DtorOp) {
      continue;
    }

    Ret.push_back(DtorOp);
  }

  return Ret;
}

[[nodiscard]] auto collectRegisteredDtorsForModule(const llvm::Module &Mod) {
  // NOLINTNEXTLINE
  llvm::SmallVector<std::pair<llvm::FunctionCallee, llvm::Value *>, 4>
      RegisteredDtors, RegisteredLocalStaticDtors;

  auto *CxaAtExitFn = Mod.getFunction("__cxa_atexit");
  if (!CxaAtExitFn) {
    return RegisteredDtors;
  }

  for (auto *User : CxaAtExitFn->users()) {
    auto *Call = llvm::dyn_cast<llvm::CallBase>(User);
    if (!Call) {
      continue;
    }

    auto *DtorOp = llvm::dyn_cast_or_null<llvm::Function>(
        Call->getArgOperand(0)->stripPointerCastsAndAliases());
    auto *DtorArgOp = Call->getArgOperand(1)->stripPointerCastsAndAliases();

    if (!DtorOp || !DtorArgOp) {
      continue;
    }

    if (Call->getFunction()->getName().contains("__cxx_global_var_init")) {
      RegisteredDtors.emplace_back(DtorOp, DtorArgOp);
    } else {
      RegisteredLocalStaticDtors.emplace_back(DtorOp, DtorArgOp);
    }
  }

  // Destructors of local static variables are registered last, no matter where
  // they are declared in the source code
  RegisteredDtors.append(RegisteredLocalStaticDtors.begin(),
                         RegisteredLocalStaticDtors.end());

  return RegisteredDtors;
}

[[nodiscard]] std::string getReducedModuleName(const llvm::Module &M) {
  auto Name = M.getName().str();
  if (auto Idx = Name.find_last_of('/'); Idx != std::string::npos) {
    Name.erase(0, Idx + 1);
  }

  return Name;
}

[[nodiscard]] llvm::Function *createDtorCallerForModule(
    llvm::Module &Mod,
    llvm::ArrayRef<std::pair<llvm::FunctionCallee, llvm::Value *>>
        RegisteredDtors,
    llvm::ArrayRef<llvm::FunctionCallee> RegisteredDtorsAtExit) {

  auto *PhasarDtorCaller = llvm::cast<llvm::Function>(
      Mod.getOrInsertFunction((GlobalCtorsDtorsModel::DtorsCallerName +
                               llvm::Twine('.') + getReducedModuleName(Mod))
                                  .str(),
                              llvm::Type::getVoidTy(Mod.getContext()))
          .getCallee());

  auto *BB =
      llvm::BasicBlock::Create(Mod.getContext(), "entry", PhasarDtorCaller);

  llvm::IRBuilder<> IRB(BB);

  for (auto FunCallee : llvm::reverse(RegisteredDtorsAtExit)) {
    IRB.CreateCall(FunCallee, {});
  }

  for (auto [FunCallee, Arg] : llvm::reverse(RegisteredDtors)) {
    assert(FunCallee.getFunctionType()->getNumParams() == 1);
    auto *ExpectedArgType = FunCallee.getFunctionType()->getParamType(0);

    if (Arg->getType() != ExpectedArgType) {
      if (!Arg->getType()->canLosslesslyBitCastTo(ExpectedArgType)) {
        PHASAR_LOG_LEVEL(
            WARNING,
            "Detected registered dtor with incompatible signature: Function "
                << FunCallee.getCallee()->getName() << " passed parameter "
                << llvmIRToString(Arg) << " of incompatible type: Expected "
                << llvmTypeToString(ExpectedArgType, true) << " vs Got"
                << llvmTypeToString(Arg->getType(), true));
        continue;
      }
      Arg = IRB.CreateBitOrPointerCast(Arg, ExpectedArgType);
    }
    IRB.CreateCall(FunCallee, {Arg});
  }

  IRB.CreateRetVoid();

  return PhasarDtorCaller;
}

[[nodiscard]] llvm::Function *collectRegisteredDtors(
    std::multimap<size_t, llvm::Function *, std::greater<>> &GlobalDtors,
    llvm::Module &Mod) {
  PHASAR_LOG_LEVEL_CAT(DEBUG, "GlobalCtorsDtorsModel",
                       "Collect Registered Dtors for Module " << Mod.getName());

  auto RegisteredDtors = collectRegisteredDtorsForModule(Mod);
  auto RegisteredDtorsAtExit = collectRegisteredDtorsAtExit(Mod);

  if (RegisteredDtors.empty() && RegisteredDtorsAtExit.empty()) {
    return nullptr;
  }

  PHASAR_LOG_LEVEL_CAT(DEBUG, "GlobalCtorsDtorsModel",
                       "> Found " << RegisteredDtors.size()
                                  << " Registered Dtors");

  auto *RegisteredDtorCaller =
      createDtorCallerForModule(Mod, RegisteredDtors, RegisteredDtorsAtExit);

  GlobalDtors.emplace(0, RegisteredDtorCaller);
  return RegisteredDtorCaller;
}

[[nodiscard]] std::pair<llvm::Function *, bool> buildCRuntimeGlobalDtorsModel(
    llvm::Module &M,
    const std::multimap<size_t, llvm::Function *, std::greater<>>
        &GlobalDtors) {
  if (GlobalDtors.size() == 1) {
    return {GlobalDtors.begin()->second, false};
  }

  auto &Ctx = M.getContext();
  auto *Cleanup = llvm::cast<llvm::Function>(
      M.getOrInsertFunction(GlobalCtorsDtorsModel::DtorModelName,
                            llvm::Type::getVoidTy(Ctx))
          .getCallee());

  auto *EntryBB = llvm::BasicBlock::Create(Ctx, "entry", Cleanup);

  llvm::IRBuilder<> IRB(EntryBB);

  /// Call all statically/dynamically registered dtors

  for (auto [unused, Dtor] : GlobalDtors) {
    assert(Dtor);
    assert(Dtor->arg_empty());
    IRB.CreateCall(Dtor);
  }

  IRB.CreateRetVoid();

  return {Cleanup, true};
}

/// Produces the values that an entry point receives from its (unknown) callers
/// outside of the analyzed module
class NonDetValueBuilder {
public:
  explicit NonDetValueBuilder(llvm::Module &M) noexcept : M(&M) {}

  llvm::Value *createValue(llvm::IRBuilder<> &IRB, llvm::Type *Ty) {
    auto &Producer = Producers[Ty];
    if (!Producer) {
      // Deliberately no memory or speculation attributes: two calls to the
      // same producer must not be merged into one value, as that would make
      // unrelated arguments must-alias.
      Producer =
          M->getOrInsertFunction((GlobalCtorsDtorsModel::NonDetValuePrefix +
                                  llvm::Twine('.') + llvmTypeToString(Ty, true))
                                     .str(),
                                 Ty);
    }
    return IRB.CreateCall(Producer);
  }

  /// Creates the argument for parameter no. ArgNo of UEntry. Parameters that
  /// are passed indirectly get the buffer that the caller is required to
  /// provide.
  llvm::Value *createArgument(llvm::IRBuilder<> &IRB,
                              const llvm::Function &UEntry, unsigned ArgNo) {
    if (auto *ByValTy = UEntry.getParamByValType(ArgNo)) {
      auto *Buffer = IRB.CreateAlloca(ByValTy);
      IRB.CreateStore(createValue(IRB, ByValTy), Buffer);
      return Buffer;
    }

    if (auto *SRetTy = UEntry.getParamStructRetType(ArgNo)) {
      return IRB.CreateAlloca(SRetTy);
    }

    return createValue(IRB, UEntry.getArg(ArgNo)->getType());
  }

private:
  llvm::Module *M;
  llvm::DenseMap<llvm::Type *, llvm::FunctionCallee> Producers;
};

/// Gives the declaration of exit() a body that runs the global destructors, so
/// that existing calls to exit() reach them through the regular call-graph
/// construction.
[[nodiscard]] llvm::Function *buildExitModel(llvm::Module &M,
                                             llvm::Function *GlobalCleanupFn) {
  auto *ExitFn = M.getFunction("exit");
  if (!ExitFn || !ExitFn->isDeclaration()) {
    return nullptr;
  }

  auto *BB = llvm::BasicBlock::Create(M.getContext(), "entry", ExitFn);
  llvm::IRBuilder<> IRB(BB);
  IRB.CreateCall(GlobalCleanupFn);
  // XXX: Do we need a call to abort() here? GlobalCleanupFn *does* actually
  // return...
  IRB.CreateUnreachable();

  return ExitFn;
}

} // namespace

llvm::DenseSet<const llvm::Function *>
GlobalCtorsDtorsModel::collectGlobalCtorsDtors(const llvm::Module &M) {
  llvm::DenseSet<const llvm::Function *> Ret;
  const auto Insert = [&Ret](size_t /*Prio*/, llvm::Function *Fun) {
    Ret.insert(Fun);
  };
  forEachGlobalCtorDtor(M, GlobalCtorsName, Insert);
  forEachGlobalCtorDtor(M, GlobalDtorsName, Insert);
  return Ret;
}

llvm::Function *GlobalCtorsDtorsModel::buildModel(
    LLVMProjectIRDB &IRDB, llvm::ArrayRef<llvm::Function *> UserEntryPoints) {
  auto &M = *IRDB.getModule();
  auto GlobalCtors = collectGlobalCtors(M);
  auto GlobalDtors = collectGlobalDtors(M);
  auto *RegisteredDtorCaller = collectRegisteredDtors(GlobalDtors, M);
  if (RegisteredDtorCaller) {
    IRDB.insertFunction(RegisteredDtorCaller);
  }

  auto [GlobalCleanupFn, Inserted] =
      buildCRuntimeGlobalDtorsModel(M, GlobalDtors);
  if (Inserted) {
    IRDB.insertFunction(GlobalCleanupFn);
  }

  if (!GlobalDtors.empty()) {
    if (auto *ExitFn = buildExitModel(M, GlobalCleanupFn)) {
      IRDB.insertFunction(ExitFn);
    }
  }

  auto &Ctx = M.getContext();
  auto *GlobModel = llvm::cast<llvm::Function>(
      M.getOrInsertFunction(ModelName,
                            /*retTy*/
                            llvm::Type::getVoidTy(Ctx),
                            /*argc*/
                            llvm::Type::getInt32Ty(Ctx),
                            /*argv*/
                            llvm::PointerType::get(Ctx, 0))
          .getCallee());

  scope_exit FinalizeModel = [&] {
    IRDB.insertFunction(GlobModel);
    ModulesToSlotTracker::updateMSTForModule(&M);
  };

  auto *EntryBB = llvm::BasicBlock::Create(Ctx, "entry", GlobModel);

  llvm::IRBuilder<> IRB(EntryBB);

  /// First, call all global ctors

  for (auto [unused, Ctor] : GlobalCtors) {
    assert(Ctor != nullptr);
    assert(Ctor->arg_empty());

    IRB.CreateCall(Ctor);
  }

  /// After all ctors have been called, now go for the user-defined entrypoints

  NonDetValueBuilder NonDet(M);

  const auto CallUEntry = [&](llvm::Function *UEntry) {
    auto NumArgs = UEntry->arg_size();
    bool IsMain = UEntry->getName() == "main";

    llvm::SmallVector<llvm::Value *, 4> Args;
    Args.reserve(NumArgs);

    for (unsigned ArgNo = 0; ArgNo != NumArgs; ++ArgNo) {
      auto *ArgTy = UEntry->getArg(ArgNo)->getType();
      // main takes argc and argv from the model's own signature; everything
      // else comes from a caller we cannot see.
      if (IsMain && ArgNo < 2 && ArgTy == GlobModel->getArg(ArgNo)->getType()) {
        Args.push_back(GlobModel->getArg(ArgNo));
        continue;
      }

      Args.push_back(NonDet.createArgument(IRB, *UEntry, ArgNo));
    }

    auto *Call = IRB.CreateCall(UEntry, Args);

    for (unsigned ArgNo = 0; ArgNo != NumArgs; ++ArgNo) {
      for (const auto &Attr : UEntry->getAttributes().getParamAttrs(ArgNo)) {
        Call->addParamAttr(ArgNo, Attr);
      }
    }
  };

  if (UserEntryPoints.size() <= 1) {
    if (UserEntryPoints.empty()) {
      PHASAR_LOG_LEVEL(WARNING,
                       "No entry points to call from the global model; only "
                       "global ctors and dtors are modeled");
    } else {
      CallUEntry(UserEntryPoints.front());
    }

    IRB.CreateCall(GlobalCleanupFn);
    IRB.CreateRetVoid();
    return GlobModel;
  }

  auto UEntrySelectorFn =
      M.getOrInsertFunction(UserEntrySelectorName, llvm::Type::getInt32Ty(Ctx));

  auto *UEntrySelector = IRB.CreateCall(UEntrySelectorFn);

  auto *DefaultBB = llvm::BasicBlock::Create(Ctx, "invalid", GlobModel);
  auto *SwitchEnd = llvm::BasicBlock::Create(Ctx, "switchEnd", GlobModel);

  auto *UEntrySwitch =
      IRB.CreateSwitch(UEntrySelector, DefaultBB, UserEntryPoints.size());

  IRB.SetInsertPoint(DefaultBB);
  IRB.CreateUnreachable();

  for (const auto &[Idx, UEntry] : llvm::enumerate(UserEntryPoints)) {
    auto *BB =
        llvm::BasicBlock::Create(Ctx, "call." + UEntry->getName(), GlobModel);
    IRB.SetInsertPoint(BB);
    CallUEntry(UEntry);
    IRB.CreateBr(SwitchEnd);

    UEntrySwitch->addCase(IRB.getInt32(uint32_t(Idx)), BB);
  }

  /// After the selected user-entries have returned, the program exists, so call
  /// the global dtors here:

  IRB.SetInsertPoint(SwitchEnd);
  IRB.CreateCall(GlobalCleanupFn);
  IRB.CreateRetVoid();

  return GlobModel;
}

llvm::Function *
GlobalCtorsDtorsModel::buildModel(LLVMProjectIRDB &IRDB,
                                  llvm::ArrayRef<std::string> UserEntryPoints) {
  auto UserEntryPointFns = getEntryFunctionsMut(IRDB, UserEntryPoints);
  return buildModel(IRDB, UserEntryPointFns);
}

bool GlobalCtorsDtorsModel::isPhasarGenerated(
    const llvm::Function &F) noexcept {
  if (!F.hasName()) {
    return false;
  }

  llvm::StringRef FunctionName = F.getName();
  const auto Cases = {ModelName, DtorModelName, DtorsCallerName,
                      UserEntrySelectorName, NonDetValuePrefix};
  return llvm::any_of(Cases, [FunctionName](llvm::StringLiteral Case) {
    return FunctionName.starts_with(Case);
  });
}
