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
#include "llvm/ADT/StringSwitch.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"

#include <iterator>
#include <optional>
#include <string>

namespace {

struct CallbackSpec {
  int CalleeArg = -1;
  llvm::SmallVector<int, 4> CallbackArgs;
  bool PassVarArgs = false;
};

using CallbackSpecs = llvm::SmallVector<CallbackSpec, 1>;

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

CallbackSpecs getKnownCallbackSpecs(llvm::StringRef BrokerName) {
  CallbackSpecs Specs;

  auto Known = llvm::StringSwitch<int>(BrokerName)
                   .Case("pthread_create", 0)
                   .Case("__kmpc_fork_call", 1)
                   .Case("__kmpc_fork_teams", 1)
                   .Default(-1);

  switch (Known) {
  case 0:
    // int pthread_create(..., void *(*start_routine)(void *), void *arg)
    Specs.push_back(CallbackSpec{2, {3}, false});
    break;
  case 1:
    // void __kmpc_fork_call/__kmpc_fork_teams(ident_t *, i32 argc,
    //                                         microtask_t, ...)
    //
    // OpenMP passes gtid and bound pointers before the user payload.
    Specs.push_back(CallbackSpec{2, {-1, -1}, true});
    break;
  default:
    break;
  }

  return Specs;
}

CallbackSpecs getCallbackSpecs(const llvm::Function &Broker) {
  auto Specs = parseCallbackMetadata(Broker);
  if (!Specs.empty()) {
    return Specs;
  }

  return getKnownCallbackSpecs(Broker.getName());
}

const llvm::Function *getDirectFunction(const llvm::Value *V) {
  if (!V) {
    return nullptr;
  }

  return llvm::dyn_cast<llvm::Function>(V->stripPointerCastsAndAliases());
}

llvm::Type *getPointerType(llvm::LLVMContext &Ctx) {
  return llvm::PointerType::get(Ctx, 0);
}

llvm::Type *getCallbackArgType(const llvm::CallBase &BrokerCall, int ArgNo) {
  if (ArgNo < 0 || static_cast<unsigned>(ArgNo) >= BrokerCall.arg_size()) {
    return getPointerType(BrokerCall.getContext());
  }

  return BrokerCall.getArgOperand(static_cast<unsigned>(ArgNo))->getType();
}

void appendCallbackParamTypes(llvm::SmallVectorImpl<llvm::Type *> &ParamTys,
                              const llvm::CallBase &BrokerCall,
                              const CallbackSpec &Spec) {
  for (int ArgNo : Spec.CallbackArgs) {
    ParamTys.push_back(getCallbackArgType(BrokerCall, ArgNo));
  }

  if (Spec.PassVarArgs) {
    unsigned FirstVarArg = BrokerCall.getFunctionType()->getNumParams();
    for (unsigned ArgNo = FirstVarArg; ArgNo < BrokerCall.arg_size(); ++ArgNo) {
      ParamTys.push_back(BrokerCall.getArgOperand(ArgNo)->getType());
    }
  }
}

llvm::FunctionType *inferKnownCallbackType(const llvm::Function &Broker,
                                           const llvm::CallBase &BrokerCall,
                                           const CallbackSpec &Spec) {
  auto Known = llvm::StringSwitch<int>(Broker.getName())
                   .Case("pthread_create", 0)
                   .Case("__kmpc_fork_call", 1)
                   .Case("__kmpc_fork_teams", 1)
                   .Default(-1);

  switch (Known) {
  case 0:
    return llvm::FunctionType::get(getPointerType(Broker.getContext()),
                                   {getPointerType(Broker.getContext())},
                                   /*isVarArg*/ false);
  case 1: {
    llvm::SmallVector<llvm::Type *, 8> ParamTys;
    appendCallbackParamTypes(ParamTys, BrokerCall, Spec);
    return llvm::FunctionType::get(llvm::Type::getVoidTy(Broker.getContext()),
                                   ParamTys, /*isVarArg*/ false);
  }
  default:
    return nullptr;
  }
}

llvm::FunctionType *inferMetadataCallbackType(const llvm::Function &Broker,
                                              const llvm::CallBase &BrokerCall,
                                              const CallbackSpec &Spec) {
  llvm::SmallVector<llvm::Type *, 8> ParamTys;
  appendCallbackParamTypes(ParamTys, BrokerCall, Spec);
  return llvm::FunctionType::get(llvm::Type::getVoidTy(Broker.getContext()),
                                 ParamTys, /*isVarArg*/ false);
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
    if (auto *KnownCBTy = inferKnownCallbackType(Broker, BrokerCall, Spec)) {
      return KnownCBTy;
    }

    return inferMetadataCallbackType(Broker, BrokerCall, Spec);
  }

  return CallbackFn->getFunctionType();
}

llvm::Value *adaptValue(llvm::IRBuilder<> &IRB, llvm::Value *Value,
                        llvm::Type *ExpectedTy) {
  if (Value->getType() == ExpectedTy) {
    return Value;
  }

  if (Value->getType()->isPointerTy() && ExpectedTy->isPointerTy()) {
    return IRB.CreatePointerCast(Value, ExpectedTy);
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
  return llvm::PoisonValue::get(ExpectedTy);
}

llvm::SmallVector<llvm::Value *, 4>
collectCallbackArgs(llvm::IRBuilder<> &IRB, llvm::Function &Model,
                    const llvm::CallBase &BrokerCall, llvm::FunctionType &CBTy,
                    const CallbackSpec &Spec) {
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

    Args.push_back(GetBrokerArg(static_cast<unsigned>(ArgNo)));
  }

  if (Spec.PassVarArgs) {
    unsigned FirstVarArg = BrokerCall.getFunctionType()->getNumParams();
    for (unsigned ArgNo = FirstVarArg; ArgNo < BrokerCall.arg_size(); ++ArgNo) {
      Args.push_back(GetBrokerArg(ArgNo));
    }
  }

  Args.resize(CBTy.getNumParams(), nullptr);

  for (unsigned Idx = 0; Idx < CBTy.getNumParams(); ++Idx) {
    auto *ExpectedTy = CBTy.getParamType(Idx);
    if (!Args[Idx]) {
      Args[Idx] = llvm::PoisonValue::get(ExpectedTy);
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
                                      llvm::Function &Broker,
                                      llvm::CallBase &BrokerCall) {
  llvm::SmallVector<llvm::Value *, 8> Args;
  Args.reserve(BrokerCall.arg_size());
  for (auto &Arg : Model.args()) {
    Args.push_back(&Arg);
  }

  auto *Call = IRB.CreateCall(Broker.getFunctionType(), &Broker, Args);
  Call->setCallingConv(BrokerCall.getCallingConv());
  return Call;
}

void createCallbackCall(llvm::IRBuilder<> &IRB, llvm::Function &Model,
                        const llvm::Function &Broker,
                        llvm::CallBase &BrokerCall, const CallbackSpec &Spec) {
  auto *CBTy = inferCallbackType(Broker, BrokerCall, Spec);
  if (!CBTy) {
    PHASAR_LOG_LEVEL(WARNING, "Cannot infer callback function type for "
                                  << psr::llvmIRToString(&BrokerCall));
    return;
  }

  auto CalleeArg = static_cast<unsigned>(Spec.CalleeArg);
  llvm::Value *CallbackCallee = Model.getArg(CalleeArg);
  if (const auto *DirectCallback =
          getDirectFunction(BrokerCall.getArgOperand(CalleeArg))) {
    CallbackCallee = const_cast<llvm::Function *>(DirectCallback);
  }
  auto Args = collectCallbackArgs(IRB, Model, BrokerCall, *CBTy, Spec);
  IRB.CreateCall(CBTy, CallbackCallee, Args);
}

llvm::Function *createModel(psr::LLVMProjectIRDB &IRDB, llvm::Function &Broker,
                            llvm::CallBase &BrokerCall,
                            llvm::ArrayRef<CallbackSpec> Specs) {
  auto *ModelTy = createModelType(BrokerCall);
  auto *Model = llvm::Function::Create(
      ModelTy, llvm::GlobalValue::PrivateLinkage,
      createModelName(IRDB, Broker, BrokerCall), IRDB.getModule());

  auto *EntryBB = llvm::BasicBlock::Create(Model->getContext(), "entry", Model);
  llvm::IRBuilder<> IRB(EntryBB);

  auto *BrokerRet = createOriginalBrokerCall(IRB, *Model, Broker, BrokerCall);

  for (const auto &Spec : Specs) {
    createCallbackCall(IRB, *Model, Broker, BrokerCall, Spec);
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
  llvm::SmallVector<std::pair<llvm::CallBase *, llvm::Function *>, 8>
      BrokerCalls;

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
      if (!Broker || getCallbackSpecs(*Broker).empty()) {
        continue;
      }

      BrokerCalls.emplace_back(Call, Broker);
    }
  }

  for (auto [Call, Broker] : BrokerCalls) {
    auto Specs = getCallbackSpecs(*Broker);
    auto *Model = createModel(IRDB, *Broker, *Call, Specs);
    Call->setCalledFunction(Model->getFunctionType(), Model);
  }

  if (!BrokerCalls.empty()) {
    PHASAR_LOG_LEVEL_CAT(INFO, "LLVMBasedICFG",
                         "Rewritten " << BrokerCalls.size()
                                      << " extern callback call-site(s)");
  }

  return BrokerCalls.size();
}

bool ExternCallbackModel::isPhasarGenerated(const llvm::Function &F) noexcept {
  return F.hasName() && F.getName().startswith(ModelPrefix);
}

} // namespace psr
