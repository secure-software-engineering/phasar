/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/IRBuilder.h"

#include <functional>

namespace psr {
template <typename MapTy>
static void insertGlobalCtorsDtorsImpl(MapTy &Into, const llvm::Module &M,
                                       llvm::StringRef Fun) {
  const auto *Gtors = M.getGlobalVariable(Fun);
  if (Gtors == nullptr) {
    return;
  }

  if (const auto *FunArray = llvm::dyn_cast<llvm::ArrayType>(
          Gtors->getType()->getPointerElementType())) {
    if (const auto *ConstFunArray =
            llvm::dyn_cast<llvm::ConstantArray>(Gtors->getInitializer())) {
      for (const auto &Op : ConstFunArray->operands()) {
        if (const auto *FunDesc = llvm::dyn_cast<llvm::ConstantStruct>(Op)) {
          auto *Fun = llvm::dyn_cast<llvm::Function>(FunDesc->getOperand(1));
          const auto *Prio =
              llvm::dyn_cast<llvm::ConstantInt>(FunDesc->getOperand(0));
          if (Fun && Prio) {
            auto PrioInt = size_t(Prio->getLimitedValue(SIZE_MAX));
            Into.emplace(PrioInt, Fun);
          }
        }
      }
    }
  }
}

static std::multimap<size_t, llvm::Function *>
collectGlobalCtors(const llvm::Module &Mod) {
  std::multimap<size_t, llvm::Function *> Ret;
  insertGlobalCtorsDtorsImpl(Ret, Mod, "llvm.global_ctors");
  return Ret;
}

static std::multimap<size_t, llvm::Function *, std::greater<>>
collectGlobalDtors(const llvm::Module &Mod) {
  std::multimap<size_t, llvm::Function *, std::greater<>> Ret;
  insertGlobalCtorsDtorsImpl(Ret, Mod, "llvm.global_dtors");
  return Ret;
}

static llvm::SmallVector<std::pair<llvm::FunctionCallee, llvm::Value *>, 4>
collectRegisteredDtorsForModule(const llvm::Module &Mod) {
  // NOLINTNEXTLINE
  llvm::SmallVector<std::pair<llvm::FunctionCallee, llvm::Value *>, 4>
      RegisteredDtors, RegisteredLocalStaticDtors;

  auto *CxaAtExitFn = Mod.getFunction("__cxa_atexit");
  if (!CxaAtExitFn) {
    return RegisteredDtors;
  }

  auto getConstantBitcastArgument = // NOLINT
      [](llvm::Value *V) -> llvm::Value * {
    auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(V);
    if (!CE) {
      return V;
    }

    return CE->getOperand(0);
  };

  for (auto *User : CxaAtExitFn->users()) {
    auto *Call = llvm::dyn_cast<llvm::CallBase>(User);
    if (!Call) {
      continue;
    }

    auto *DtorOp = llvm::dyn_cast_or_null<llvm::Function>(
        getConstantBitcastArgument(Call->getArgOperand(0)));
    auto *DtorArgOp = getConstantBitcastArgument(Call->getArgOperand(1));

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

static std::string getReducedModuleName(const llvm::Module &M) {
  auto Name = M.getName().str();
  if (auto Idx = Name.find_last_of('/'); Idx != std::string::npos) {
    Name.erase(0, Idx + 1);
  }

  return Name;
}

static llvm::Function *createDtorCallerForModule(
    llvm::Module &Mod,
    const llvm::SmallVectorImpl<std::pair<llvm::FunctionCallee, llvm::Value *>>
        &RegisteredDtors) {

  auto *PhasarDtorCaller = llvm::cast<llvm::Function>(
      Mod.getOrInsertFunction("__psrGlobalDtorsCaller." +
                                  getReducedModuleName(Mod),
                              llvm::Type::getVoidTy(Mod.getContext()))
          .getCallee());

  auto *BB =
      llvm::BasicBlock::Create(Mod.getContext(), "entry", PhasarDtorCaller);

  llvm::IRBuilder<> IRB(BB);

  for (auto It = RegisteredDtors.rbegin(), End = RegisteredDtors.rend();
       It != End; ++It) {
    IRB.CreateCall(It->first, {It->second});
  }

  IRB.CreateRetVoid();

  return PhasarDtorCaller;
}

[[nodiscard]] static llvm::Function *collectRegisteredDtors(
    std::multimap<size_t, llvm::Function *, std::greater<>> &GlobalDtors,
    llvm::Module &Mod) {
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                       "Collect Registered Dtors for Module " << Mod.getName());

  auto RegisteredDtors = collectRegisteredDtorsForModule(Mod);

  if (RegisteredDtors.empty()) {
    return nullptr;
  }

  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                       "> Found " << RegisteredDtors.size()
                                  << " Registered Dtors");

  auto *RegisteredDtorCaller = createDtorCallerForModule(Mod, RegisteredDtors);
  // auto It =
  GlobalDtors.emplace(0, RegisteredDtorCaller);
  // GlobalDtorFn.try_emplace(RegisteredDtorCaller, it);
  // GlobalRegisteredDtorsCaller.try_emplace(Mod, RegisteredDtorCaller);
  return RegisteredDtorCaller;
}

static std::pair<llvm::Function *, bool> buildCRuntimeGlobalDtorsModel(
    llvm::Module &M,
    const std::multimap<size_t, llvm::Function *, std::greater<>>
        &GlobalDtors) {
  if (GlobalDtors.size() == 1) {
    return {GlobalDtors.begin()->second, false};
  }

  auto &CTX = M.getContext();
  auto *Cleanup = llvm::cast<llvm::Function>(
      M.getOrInsertFunction("__psrCRuntimeGlobalDtorsModel",
                            llvm::Type::getVoidTy(CTX))
          .getCallee());

  auto *EntryBB = llvm::BasicBlock::Create(CTX, "entry", Cleanup);

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

llvm::Function *LLVMBasedICFG::buildCRuntimeGlobalCtorsDtorsModel(
    llvm::Module &M, llvm::ArrayRef<llvm::Function *> UserEntryPoints) {
  auto GlobalCtors = collectGlobalCtors(M);
  auto GlobalDtors = collectGlobalDtors(M);
  auto *RegisteredDtorCaller = collectRegisteredDtors(GlobalDtors, M);
  if (RegisteredDtorCaller) {
    IRDB->insertFunction(RegisteredDtorCaller);
  }

  auto [GlobalCleanupFn, Inserted] =
      buildCRuntimeGlobalDtorsModel(M, GlobalDtors);
  if (Inserted) {
    IRDB->insertFunction(GlobalCleanupFn);
  }

  auto &CTX = M.getContext();
  auto *GlobModel = llvm::cast<llvm::Function>(
      M.getOrInsertFunction(GlobalCRuntimeModelName,
                            /*retTy*/
                            llvm::Type::getVoidTy(CTX),
                            /*argc*/
                            llvm::Type::getInt32Ty(CTX),
                            /*argv*/
                            llvm::Type::getInt8PtrTy(CTX)->getPointerTo())
          .getCallee());

  auto *EntryBB = llvm::BasicBlock::Create(CTX, "entry", GlobModel);

  llvm::IRBuilder<> IRB(EntryBB);

  /// First, call all global ctors

  for (auto [unused, Ctor] : GlobalCtors) {
    assert(Ctor != nullptr);
    assert(Ctor->arg_size() == 0);

    IRB.CreateCall(Ctor);
  }

  /// After all ctors have been called, now go for the user-defined entrypoints

  assert(!UserEntryPoints.empty());

  auto CallUEntry =
      [&, GlobalCleanupFn{GlobalCleanupFn}](llvm::Function *UEntry) { // NOLINT
        switch (UEntry->arg_size()) {
        case 0:
          IRB.CreateCall(UEntry);
          break;
        case 2:
          if (UEntry->getName() != "main") {
            PHASAR_LOG_LEVEL(
                WARNING,
                "WARNING: The only entrypoint, where parameters are "
                "supported, is 'main'.\nAutomated global support for library "
                "analysis (entry-points=__ALL__) is not yet supported.");

            break;
          }

          IRB.CreateCall(UEntry, {GlobModel->getArg(0), GlobModel->getArg(1)});
          break;
        default:
          PHASAR_LOG_LEVEL(
              WARNING,
              "WARNING: Entrypoints with parameters are not supported, "
              "except for argc and argv in main.\nAutomated global support for "
              "library analysis (entry-points=__ALL__) is not yet supported.");

          break;
        }

        if (UEntry->getName() == "main") {
          ///  After the main function, we must call all global destructors...
          IRB.CreateCall(GlobalCleanupFn);
        }
      };

  if (UserEntryPoints.size() == 1) {
    auto *MainFn = *UserEntryPoints.begin();
    CallUEntry(MainFn);
    IRB.CreateRetVoid();
  } else {

    auto UEntrySelectorFn = M.getOrInsertFunction(
        "__psrCRuntimeUserEntrySelector", llvm::Type::getInt32Ty(CTX));

    auto *UEntrySelector = IRB.CreateCall(UEntrySelectorFn, {});

    auto *DefaultBB = llvm::BasicBlock::Create(CTX, "invalid", GlobModel);
    auto *SwitchEnd = llvm::BasicBlock::Create(CTX, "switchEnd", GlobModel);

    auto *UEntrySwitch =
        IRB.CreateSwitch(UEntrySelector, DefaultBB, UserEntryPoints.size());

    IRB.SetInsertPoint(DefaultBB);
    IRB.CreateUnreachable();

    unsigned Idx = 0;

    for (auto *UEntry : UserEntryPoints) {
      auto *BB =
          llvm::BasicBlock::Create(CTX, "call" + UEntry->getName(), GlobModel);
      IRB.SetInsertPoint(BB);
      CallUEntry(UEntry);
      IRB.CreateBr(SwitchEnd);

      UEntrySwitch->addCase(IRB.getInt32(Idx), BB);

      ++Idx;
    }

    /// After all user-entries have been called, we are done

    IRB.SetInsertPoint(SwitchEnd);
    IRB.CreateRetVoid();
  }

  IRDB->insertFunction(GlobModel);
  ModulesToSlotTracker::updateMSTForModule(&M);

  return GlobModel;
}
} // namespace psr
