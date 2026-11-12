/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/ExternCallbackModel.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <string>

namespace {

enum class CallbackTypeKind { VoidResult, PthreadCreate, IntResult };

struct CallbackSpec {
  int CalleeArg = -1;
  /// Broker arguments passed to the callback; -1: none (undef)
  llvm::SmallVector<int, 4> CallbackArgs;
  bool PassVarArgs = false;
  CallbackTypeKind TypeKind = CallbackTypeKind::VoidResult;
  /// qsort-like brokers: a callback argument ArrayArg is a pointer to an
  /// element of that array, with the element count (or a pointer to it) and
  /// size in these broker arguments
  int ArrayArg = -1;
  int NumElemsArg = -1;
  int ElemSizeArg = -1;
  /// The broker stores an array of pointers through ArrayArg and returns its
  /// length, e.g. scandir's namelist
  bool ArrayIsOut = false;
  /// The callback gets an element's value instead of its address
  bool LoadElement = false;
};

using CallbackSpecs = llvm::SmallVector<CallbackSpec, 1>;

struct RewriteCandidate {
  llvm::CallBase *Call = nullptr;
  llvm::Function *Broker = nullptr;
  CallbackSpecs Specs;
};

std::optional<int64_t> getIntFromMetadata(const llvm::Metadata *MD) {
  const auto *ConstMD = llvm::dyn_cast_or_null<llvm::ConstantAsMetadata>(MD);
  if (!ConstMD) {
    return std::nullopt;
  }

  const auto *ConstInt = llvm::dyn_cast<llvm::ConstantInt>(ConstMD->getValue());
  if (!ConstInt) {
    return std::nullopt;
  }

  return ConstInt->getSExtValue();
}

std::optional<bool> getBoolFromMetadata(const llvm::Metadata *MD) {
  auto Int = getIntFromMetadata(MD);
  if (!Int) {
    return std::nullopt;
  }

  return *Int != 0;
}

std::optional<CallbackSpec> parseCallbackTuple(const llvm::MDNode &Tuple) {
  if (Tuple.getNumOperands() < 2) {
    return std::nullopt;
  }

  auto CalleeArg = getIntFromMetadata(Tuple.getOperand(0).get());
  auto PassVarArgs =
      getBoolFromMetadata(Tuple.getOperand(Tuple.getNumOperands() - 1).get());
  if (!CalleeArg || !PassVarArgs) {
    return std::nullopt;
  }

  CallbackSpec Spec;
  Spec.CalleeArg = static_cast<int>(*CalleeArg);
  Spec.PassVarArgs = *PassVarArgs;

  for (unsigned Idx = 1; Idx + 1 < Tuple.getNumOperands(); ++Idx) {
    auto CallbackArg = getIntFromMetadata(Tuple.getOperand(Idx).get());
    if (!CallbackArg) {
      return std::nullopt;
    }
    Spec.CallbackArgs.push_back(static_cast<int>(*CallbackArg));
  }

  return Spec;
}

CallbackSpecs parseCallbackMetadata(const llvm::Function &Broker) {
  CallbackSpecs Specs;
  const auto *CallbackMD = Broker.getMetadata("callback");
  if (!CallbackMD) {
    return Specs;
  }

  for (const auto &Op : CallbackMD->operands()) {
    const auto *Tuple = llvm::dyn_cast_or_null<llvm::MDNode>(Op.get());
    if (!Tuple) {
      continue;
    }

    if (auto Spec = parseCallbackTuple(*Tuple)) {
      Specs.push_back(std::move(*Spec));
    }
  }

  return Specs;
}

const llvm::StringMap<CallbackSpecs> &knownCallbackSpecs() {
  static const auto Known = [] {
    using enum CallbackTypeKind;
    llvm::StringMap<CallbackSpecs> Ret;
    const auto Add = [&Ret](std::initializer_list<llvm::StringRef> Names,
                            const CallbackSpecs &Specs) {
      for (auto Name : Names) {
        Ret[Name] = Specs;
      }
    };
    const auto Plain = [](int CalleeArg, llvm::SmallVector<int, 4> Args,
                          CallbackTypeKind Kind) {
      return CallbackSpec{
          .CalleeArg = CalleeArg,
          .CallbackArgs = std::move(Args),
          .TypeKind = Kind,
      };
    };
    // compar(Args...) with pointers to elements of the array at ArrayArg,
    // whose count and element size follow it
    const auto Compare = [](int CalleeArg, llvm::SmallVector<int, 4> Args,
                            int ArrayArg) {
      return CallbackSpec{
          .CalleeArg = CalleeArg,
          .CallbackArgs = std::move(Args),
          .TypeKind = IntResult,
          .ArrayArg = ArrayArg,
          .NumElemsArg = ArrayArg + 1,
          .ElemSizeArg = ArrayArg + 2,
      };
    };
    // filter(entry) and compar(&entry, &entry) on the list stored through
    // NameList
    const auto Scandir = [](int NameList) {
      return CallbackSpecs{
          CallbackSpec{
              .CalleeArg = NameList + 1,
              .CallbackArgs = {NameList},
              .TypeKind = IntResult,
              .ArrayArg = NameList,
              .ArrayIsOut = true,
              .LoadElement = true,
          },
          CallbackSpec{
              .CalleeArg = NameList + 2,
              .CallbackArgs = {NameList, NameList},
              .TypeKind = IntResult,
              .ArrayArg = NameList,
              .ArrayIsOut = true,
          },
      };
    };

    // int pthread_create(..., void *(*start_routine)(void *), void *arg)
    Add({"pthread_create"}, {Plain(2, {3}, PthreadCreate)});
    // int thrd_create(thrd_t *, int (*func)(void *), void *arg)
    Add({"thrd_create"}, {Plain(1, {2}, IntResult)});
    // void __kmpc_fork_call/__kmpc_fork_teams(ident_t *, i32 argc,
    //                                         microtask_t, ...)
    //
    // OpenMP passes gtid and bound pointers before the user payload.
    Add({"__kmpc_fork_call", "__kmpc_fork_teams"}, {CallbackSpec{
                                                       .CalleeArg = 2,
                                                       .CallbackArgs = {-1, -1},
                                                       .PassVarArgs = true,
                                                       .TypeKind = VoidResult,
                                                   }});
    // void qsort(void *base, size_t nmemb, size_t size,
    //            int (*compar)(const void *, const void *))
    Add({"qsort"}, {Compare(3, {0, 0}, 0)});
    // qsort_r(..., int (*compar)(const void *, const void *, void *), arg)
    Add({"qsort_r"}, {Compare(3, {0, 0, 4}, 0)});
    // void *bsearch(const void *key, const void *base, size_t nmemb,
    //               size_t size, int (*compar)(const void *, const void *));
    // lfind and lsearch take a pointer to nmemb
    Add({"bsearch", "lfind", "lsearch"}, {Compare(4, {0, 1}, 1)});
    // int scandir(const char *, struct dirent ***namelist, filter, compar)
    Add({"scandir", "scandir64"}, Scandir(1));
    Add({"scandirat", "scandirat64"}, Scandir(2));
    // int ftw(const char *, int (*fn)(fpath, sb, typeflag), int); nftw's fn
    // also gets ftwbuf. All of them are libc's.
    Add({"ftw", "ftw64"}, {Plain(1, {-1, -1, -1}, IntResult)});
    Add({"nftw", "nftw64"}, {Plain(1, {-1, -1, -1, -1}, IntResult)});
    // int dl_iterate_phdr(int (*callback)(info, size_t, void *data), data)
    Add({"dl_iterate_phdr"}, {Plain(0, {-1, -1, 1}, IntResult)});
    return Ret;
  }();
  return Known;
}

CallbackSpecs getKnownCallbackSpecs(llvm::StringRef BrokerName) {
  const auto &Known = knownCallbackSpecs();
  if (const auto It = Known.find(BrokerName); It != Known.end()) {
    return It->second;
  }
  return {};
}

CallbackSpecs getCallbackSpecs(const llvm::Function &Broker) {
  auto Specs = parseCallbackMetadata(Broker);
  if (!Specs.empty()) {
    return Specs;
  }

  return getKnownCallbackSpecs(Broker.getName());
}

llvm::Function *getDirectFunction(llvm::Value *V) {
  if (!V) {
    return nullptr;
  }

  return llvm::dyn_cast<llvm::Function>(V->stripPointerCastsAndAliases());
}

llvm::Type *getPointerType(llvm::LLVMContext &Ctx) {
  return llvm::PointerType::get(Ctx, 0);
}

llvm::Type *getCallbackArgType(const llvm::CallBase &BrokerCall, int ArgNo) {
  if (ArgNo < 0) {
    return getPointerType(BrokerCall.getContext());
  }

  if (static_cast<unsigned>(ArgNo) >= BrokerCall.arg_size()) {
    return nullptr;
  }

  return BrokerCall.getArgOperand(static_cast<unsigned>(ArgNo))->getType();
}

bool appendCallbackParamTypes(llvm::SmallVectorImpl<llvm::Type *> &ParamTys,
                              const llvm::CallBase &BrokerCall,
                              const CallbackSpec &Spec) {
  for (int ArgNo : Spec.CallbackArgs) {
    auto *Ty = getCallbackArgType(BrokerCall, ArgNo);
    if (!Ty) {
      return false;
    }
    ParamTys.push_back(Ty);
  }

  if (Spec.PassVarArgs) {
    unsigned FirstVarArg = BrokerCall.getFunctionType()->getNumParams();
    for (unsigned ArgNo = FirstVarArg; ArgNo < BrokerCall.arg_size(); ++ArgNo) {
      ParamTys.push_back(BrokerCall.getArgOperand(ArgNo)->getType());
    }
  }

  return true;
}

llvm::FunctionType *inferFallbackCallbackType(const llvm::Function &Broker,
                                              const llvm::CallBase &BrokerCall,
                                              const CallbackSpec &Spec) {
  switch (Spec.TypeKind) {
  case CallbackTypeKind::PthreadCreate:
    return llvm::FunctionType::get(getPointerType(Broker.getContext()),
                                   {getPointerType(Broker.getContext())},
                                   /*isVarArg*/ false);
  case CallbackTypeKind::VoidResult: {
    llvm::SmallVector<llvm::Type *, 8> ParamTys;
    if (!appendCallbackParamTypes(ParamTys, BrokerCall, Spec)) {
      return nullptr;
    }
    return llvm::FunctionType::get(llvm::Type::getVoidTy(Broker.getContext()),
                                   ParamTys, /*isVarArg*/ false);
  }
  case CallbackTypeKind::IntResult: {
    llvm::SmallVector<llvm::Type *, 8> ParamTys;
    if (!appendCallbackParamTypes(ParamTys, BrokerCall, Spec)) {
      return nullptr;
    }
    return llvm::FunctionType::get(llvm::Type::getInt32Ty(Broker.getContext()),
                                   ParamTys, /*isVarArg*/ false);
  }
  }
  llvm_unreachable("Unhandled callback type kind");
}

llvm::FunctionType *inferCallbackType(const llvm::Function &Broker,
                                      const llvm::CallBase &BrokerCall,
                                      const CallbackSpec &Spec) {
  if (Spec.CalleeArg < 0 ||
      static_cast<unsigned>(Spec.CalleeArg) >= BrokerCall.arg_size()) {
    return nullptr;
  }

  const auto *CallbackFn =
      getDirectFunction(BrokerCall.getArgOperand(Spec.CalleeArg));
  if (!CallbackFn) {
    return inferFallbackCallbackType(Broker, BrokerCall, Spec);
  }

  return CallbackFn->getFunctionType();
}

bool hasBrokerArg(const llvm::CallBase &BrokerCall, int ArgNo) noexcept {
  return ArgNo >= 0 && static_cast<unsigned>(ArgNo) < BrokerCall.arg_size();
}

unsigned getCallbackArgCount(const llvm::CallBase &BrokerCall,
                             const CallbackSpec &Spec) {
  unsigned Count = Spec.CallbackArgs.size();
  if (!Spec.PassVarArgs) {
    return Count;
  }

  unsigned FirstVarArg = BrokerCall.getFunctionType()->getNumParams();
  if (FirstVarArg >= BrokerCall.arg_size()) {
    return Count;
  }
  return Count + BrokerCall.arg_size() - FirstVarArg;
}

bool hasExpectedCallbackArgCount(const llvm::FunctionType &CBTy,
                                 unsigned ArgCount) noexcept {
  if (CBTy.isVarArg()) {
    return ArgCount >= CBTy.getNumParams();
  }
  return ArgCount == CBTy.getNumParams();
}

bool validateSpec(const llvm::Function &Broker,
                  const llvm::CallBase &BrokerCall, const CallbackSpec &Spec) {
  if (!hasBrokerArg(BrokerCall, Spec.CalleeArg)) {
    PHASAR_LOG_LEVEL(WARNING, "Cannot model extern callback broker "
                                  << Broker.getName() << ": callback callee "
                                  << "argument " << Spec.CalleeArg
                                  << " is unavailable in "
                                  << psr::llvmIRToString(&BrokerCall));
    return false;
  }

  auto CalleeArg = static_cast<unsigned>(Spec.CalleeArg);
  if (!BrokerCall.getArgOperand(CalleeArg)->getType()->isPointerTy()) {
    PHASAR_LOG_LEVEL(WARNING, "Cannot model extern callback broker "
                                  << Broker.getName() << ": callback callee "
                                  << "argument " << Spec.CalleeArg
                                  << " is not a pointer in "
                                  << psr::llvmIRToString(&BrokerCall));
    return false;
  }

  bool HasMissingCallbackArgs = false;
  for (int ArgNo : Spec.CallbackArgs) {
    if (ArgNo < 0) {
      continue;
    }

    if (!hasBrokerArg(BrokerCall, ArgNo)) {
      PHASAR_LOG_LEVEL(WARNING, "Cannot model extern callback broker "
                                    << Broker.getName()
                                    << ": callback argument " << ArgNo
                                    << " is unavailable in "
                                    << psr::llvmIRToString(&BrokerCall));
      HasMissingCallbackArgs = true;
    }
  }
  if (HasMissingCallbackArgs) {
    return false;
  }

  const auto HasArray = [&] {
    if (!BrokerCall.getArgOperand(Spec.ArrayArg)->getType()->isPointerTy()) {
      return false;
    }
    if (Spec.ArrayIsOut) {
      return BrokerCall.getType()->isIntegerTy();
    }
    if (!hasBrokerArg(BrokerCall, Spec.NumElemsArg) ||
        !hasBrokerArg(BrokerCall, Spec.ElemSizeArg)) {
      return false;
    }
    const auto *NumElemsTy =
        BrokerCall.getArgOperand(Spec.NumElemsArg)->getType();
    return NumElemsTy->isIntegerTy() || NumElemsTy->isPointerTy();
  };
  if (Spec.ArrayArg >= 0 && !HasArray()) {
    PHASAR_LOG_LEVEL(WARNING, "Cannot model extern callback broker "
                                  << Broker.getName()
                                  << ": no array with element count and size "
                                  << "in " << psr::llvmIRToString(&BrokerCall));
    return false;
  }

  auto *CBTy = inferCallbackType(Broker, BrokerCall, Spec);
  if (!CBTy) {
    PHASAR_LOG_LEVEL(WARNING, "Cannot infer callback function type for "
                                  << psr::llvmIRToString(&BrokerCall));
    return false;
  }

  unsigned ArgCount = getCallbackArgCount(BrokerCall, Spec);
  if (!hasExpectedCallbackArgCount(*CBTy, ArgCount)) {
    PHASAR_LOG_LEVEL(
        WARNING, "Cannot model extern callback broker "
                     << Broker.getName() << ": expected "
                     << CBTy->getNumParams() << " callback argument(s), got "
                     << ArgCount << " in " << psr::llvmIRToString(&BrokerCall));
    return false;
  }

  return true;
}

bool validateSpecs(const llvm::Function &Broker,
                   const llvm::CallBase &BrokerCall,
                   llvm::ArrayRef<CallbackSpec> Specs) {
  for (const auto &Spec : Specs) {
    if (!validateSpec(Broker, BrokerCall, Spec)) {
      return false;
    }
  }

  return true;
}

llvm::Value *adaptValue(llvm::IRBuilder<> &IRB, llvm::Value *Value,
                        llvm::Type *ExpectedTy) {
  if (Value->getType() == ExpectedTy) {
    return Value;
  }

  if (Value->getType()->isIntegerTy() && ExpectedTy->isIntegerTy()) {
    return IRB.CreateIntCast(Value, ExpectedTy, /*isSigned*/ false);
  }

  if (Value->getType()->isPointerTy() && ExpectedTy->isIntegerTy()) {
    return IRB.CreatePtrToInt(Value, ExpectedTy);
  }

  if (Value->getType()->isIntegerTy() && ExpectedTy->isPointerTy()) {
    return IRB.CreateIntToPtr(Value, ExpectedTy);
  }

  PHASAR_LOG_LEVEL(WARNING, "Cannot adapt extern callback argument "
                                << psr::llvmIRToString(Value) << " to "
                                << psr::llvmTypeToString(ExpectedTy, true));
  return llvm::UndefValue::get(ExpectedTy);
}

llvm::Value *createElementPtr(llvm::IRBuilder<> &IRB, llvm::Function &Model,
                              const llvm::CallBase &BrokerCall,
                              const CallbackSpec &Spec,
                              llvm::Value *BrokerRet) {
  if (Spec.ArrayIsOut) {
    auto *PtrTy = getPointerType(Model.getContext());
    auto *Array = IRB.CreateLoad(PtrTy, Model.getArg(Spec.ArrayArg), "array");
    return IRB.CreateGEP(PtrTy, Array, BrokerRet, "elem");
  }
  uint64_t ElemSize = 1;
  if (const auto *Size = llvm::dyn_cast<llvm::ConstantInt>(
          BrokerCall.getArgOperand(Spec.ElemSizeArg));
      Size && !Size->isZero()) {
    ElemSize = Size->getZExtValue();
  }
  llvm::Value *NumElems = Model.getArg(Spec.NumElemsArg);
  if (NumElems->getType()->isPointerTy()) {
    NumElems = IRB.CreateLoad(
        Model.getParent()->getDataLayout().getIntPtrType(Model.getContext()),
        NumElems, "nmemb");
  }
  auto *ElemTy = llvm::ArrayType::get(IRB.getInt8Ty(), ElemSize);
  return IRB.CreateGEP(ElemTy, Model.getArg(Spec.ArrayArg), NumElems, "elem");
}

llvm::SmallVector<llvm::Value *, 4>
collectCallbackArgs(llvm::IRBuilder<> &IRB, llvm::Function &Model,
                    const llvm::CallBase &BrokerCall, llvm::FunctionType &CBTy,
                    const CallbackSpec &Spec, llvm::Value *BrokerRet) {
  llvm::SmallVector<llvm::Value *, 4> Args;

  auto GetBrokerArg = [&Model](unsigned ArgNo) -> llvm::Argument * {
    auto It = Model.arg_begin();
    std::advance(It, ArgNo);
    return &*It;
  };

  for (int ArgNo : Spec.CallbackArgs) {
    if (ArgNo < 0 || static_cast<unsigned>(ArgNo) >= BrokerCall.arg_size()) {
      Args.push_back(nullptr);
      continue;
    }
    if (ArgNo == Spec.ArrayArg) {
      auto *Elem = createElementPtr(IRB, Model, BrokerCall, Spec, BrokerRet);
      if (Spec.LoadElement) {
        Elem =
            IRB.CreateLoad(getPointerType(Model.getContext()), Elem, "elemval");
      }
      Args.push_back(Elem);
      continue;
    }

    Args.push_back(GetBrokerArg(static_cast<unsigned>(ArgNo)));
  }

  if (Spec.PassVarArgs) {
    unsigned FirstVarArg = BrokerCall.getFunctionType()->getNumParams();
    for (unsigned ArgNo = FirstVarArg; ArgNo < BrokerCall.arg_size(); ++ArgNo) {
      Args.push_back(GetBrokerArg(ArgNo));
    }
  }

  for (unsigned Idx = 0,
                End = std::min<unsigned>(Args.size(), CBTy.getNumParams());
       Idx < End; ++Idx) {
    auto *ExpectedTy = CBTy.getParamType(Idx);
    if (!Args[Idx]) {
      Args[Idx] = llvm::UndefValue::get(ExpectedTy);
      continue;
    }

    Args[Idx] = adaptValue(IRB, Args[Idx], ExpectedTy);
  }

  return Args;
}

llvm::FunctionType *createModelType(llvm::CallBase &BrokerCall) {
  llvm::SmallVector<llvm::Type *, 8> ParamTys;
  ParamTys.reserve(BrokerCall.arg_size());
  for (auto &Arg : BrokerCall.args()) {
    ParamTys.push_back(Arg->getType());
  }

  return llvm::FunctionType::get(BrokerCall.getType(), ParamTys,
                                 /*isVarArg*/ false);
}

std::string createModelName(const psr::LLVMProjectIRDB &IRDB,
                            const llvm::Function &Broker,
                            const llvm::CallBase &BrokerCall) {
  std::string Name;
  llvm::raw_string_ostream OS(Name);
  OS << psr::ExternCallbackModel::ModelPrefix << '.' << Broker.getName() << '.'
     << IRDB.getInstructionId(&BrokerCall);
  return Name;
}

llvm::Value *createOriginalBrokerCall(llvm::IRBuilder<> &IRB,
                                      llvm::Function &Model,
                                      llvm::CallBase &BrokerCall) {
  llvm::SmallVector<llvm::Value *, 8> Args;
  Args.reserve(BrokerCall.arg_size());
  for (auto &Arg : Model.args()) {
    Args.push_back(&Arg);
  }

  llvm::SmallVector<llvm::OperandBundleDef, 2> OperandBundles;
  BrokerCall.getOperandBundlesAsDefs(OperandBundles);

  auto *Call =
      IRB.CreateCall(BrokerCall.getFunctionType(),
                     BrokerCall.getCalledOperand(), Args, OperandBundles);
  Call->setCallingConv(BrokerCall.getCallingConv());
  Call->setAttributes(BrokerCall.getAttributes());
  Call->setDebugLoc(BrokerCall.getDebugLoc());
  Call->copyMetadata(BrokerCall);
  return Call;
}

void createCallbackCall(llvm::IRBuilder<> &IRB, llvm::Function &Model,
                        const llvm::Function &Broker,
                        llvm::CallBase &BrokerCall, const CallbackSpec &Spec,
                        llvm::Value *BrokerRet) {
  auto *CBTy = inferCallbackType(Broker, BrokerCall, Spec);
  if (!CBTy) {
    PHASAR_LOG_LEVEL(WARNING, "Cannot infer callback function type for "
                                  << psr::llvmIRToString(&BrokerCall));
    return;
  }

  auto CalleeArg = static_cast<unsigned>(Spec.CalleeArg);
  llvm::Value *CallbackCallee = Model.getArg(CalleeArg);
  if (auto *DirectCallback =
          getDirectFunction(BrokerCall.getArgOperand(CalleeArg))) {
    CallbackCallee = DirectCallback;
  }
  auto Args =
      collectCallbackArgs(IRB, Model, BrokerCall, *CBTy, Spec, BrokerRet);
  auto *Call = IRB.CreateCall(CBTy, CallbackCallee, Args);
  Call->setDebugLoc(BrokerCall.getDebugLoc());
}

llvm::Function *createModel(psr::LLVMProjectIRDB &IRDB, llvm::Function &Broker,
                            llvm::CallBase &BrokerCall,
                            llvm::ArrayRef<CallbackSpec> Specs) {
  auto *ModelTy = createModelType(BrokerCall);
  auto *Model = llvm::Function::Create(
      ModelTy, llvm::GlobalValue::PrivateLinkage,
      createModelName(IRDB, Broker, BrokerCall), IRDB.getModule());
  Model->setCallingConv(BrokerCall.getCallingConv());

  auto *EntryBB = llvm::BasicBlock::Create(Model->getContext(), "entry", Model);
  llvm::IRBuilder<> IRB(EntryBB);

  auto *BrokerRet = createOriginalBrokerCall(IRB, *Model, BrokerCall);

  for (const auto &Spec : Specs) {
    createCallbackCall(IRB, *Model, Broker, BrokerCall, Spec, BrokerRet);
  }

  if (ModelTy->getReturnType()->isVoidTy()) {
    IRB.CreateRetVoid();
  } else {
    IRB.CreateRet(adaptValue(IRB, BrokerRet, ModelTy->getReturnType()));
  }

  IRDB.insertFunction(Model);
  return Model;
}

bool shouldSkipFunction(const llvm::Function &F) {
  return F.isDeclaration() || psr::ExternCallbackModel::isPhasarGenerated(F);
}

} // namespace

namespace psr {

size_t ExternCallbackModel::rewriteCalls(LLVMProjectIRDB &IRDB) {
  llvm::SmallVector<RewriteCandidate, 8> BrokerCalls;

  for (auto &F : *IRDB.getModule()) {
    if (shouldSkipFunction(F)) {
      continue;
    }

    for (auto &Inst : llvm::instructions(F)) {
      auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst);
      if (!Call) {
        continue;
      }

      auto *Broker = llvm::dyn_cast<llvm::Function>(
          Call->getCalledOperand()->stripPointerCastsAndAliases());
      if (!Broker) {
        continue;
      }

      auto Specs = getCallbackSpecs(*Broker);
      if (Specs.empty() || !validateSpecs(*Broker, *Call, Specs)) {
        continue;
      }
      // E.g. scandir without a filter
      llvm::erase_if(Specs, [Call](const CallbackSpec &Spec) {
        return llvm::isa<llvm::ConstantPointerNull>(
            Call->getArgOperand(Spec.CalleeArg));
      });
      if (Specs.empty()) {
        continue;
      }

      BrokerCalls.push_back({Call, Broker, std::move(Specs)});
    }
  }

  for (auto &Candidate : BrokerCalls) {
    auto *Model =
        createModel(IRDB, *Candidate.Broker, *Candidate.Call, Candidate.Specs);
    Candidate.Call->setCalledFunction(Model->getFunctionType(), Model);
    Candidate.Call->setAttributes(llvm::AttributeList{});
  }

  if (!BrokerCalls.empty()) {
    PHASAR_LOG_LEVEL_CAT(INFO, "LLVMBasedICFG",
                         "Rewritten " << BrokerCalls.size()
                                      << " extern callback call-site(s)");
  }

  return BrokerCalls.size();
}

bool ExternCallbackModel::isPhasarGenerated(const llvm::Function &F) noexcept {
  return F.hasName() && F.getName().starts_with(ModelPrefix);
}

} // namespace psr
