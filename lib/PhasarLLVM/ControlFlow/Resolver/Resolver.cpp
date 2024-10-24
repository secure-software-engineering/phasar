/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Resolver.cpp
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/DTAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/NOResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/OTFResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <optional>

std::optional<unsigned> psr::getVFTIndex(const llvm::CallBase *CallSite) {
  // deal with a virtual member function
  // retrieve the vtable entry that is called
  const auto *Load =
      llvm::dyn_cast<llvm::LoadInst>(CallSite->getCalledOperand());
  if (Load == nullptr) {
    return std::nullopt;
  }
  const auto *GEP =
      llvm::dyn_cast<llvm::GetElementPtrInst>(Load->getPointerOperand());
  if (GEP == nullptr) {
    return std::nullopt;
  }
  if (auto *CI = llvm::dyn_cast<llvm::ConstantInt>(GEP->getOperand(1))) {
    return CI->getZExtValue();
  }
  return std::nullopt;
}

const llvm::DIType *psr::getReceiverType(const llvm::CallBase *CallSite) {
  if (CallSite->arg_empty() ||
      (CallSite->hasStructRetAttr() && CallSite->arg_size() < 2)) {
    return nullptr;
  }

  const auto *Receiver =
      CallSite->getArgOperand(unsigned(CallSite->hasStructRetAttr()));

  if (!Receiver->getType()->isPointerTy()) {
    return nullptr;
  }

  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Receiver)) {
    if (const auto *DITy = getDILocalVariable(Load->getPointerOperand())) {
      if (const auto *DerivedTy =
              llvm::dyn_cast<llvm::DIDerivedType>(DITy->getType())) {
        return DerivedTy->getBaseType();
      }
    }
  }

  return nullptr;
}

std::string psr::getReceiverTypeName(const llvm::CallBase *CallSite) {
  const auto *RT = getReceiverType(CallSite);
  if (RT) {
    return RT->getName().str();
  }
  return "";
}

bool psr::isConsistentCall(const llvm::CallBase *CallSite,
                           const llvm::Function *DestFun) {
  if (CallSite->arg_size() < DestFun->arg_size()) {
    return false;
  }
  if (CallSite->arg_size() != DestFun->arg_size() && !DestFun->isVarArg()) {
    return false;
  }
  if (!matchesSignature(DestFun, CallSite->getFunctionType(), false)) {
    return false;
  }
  return true;
}

bool psr::isVirtualCall(const llvm::Instruction *Inst,
                        const LLVMVFTableProvider &VTP) {
  assert(Inst != nullptr);
  const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Inst);
  if (!CallSite) {
    return false;
  }
  // check potential receiver type
  const auto *RecType = getReceiverType(CallSite);
  if (!RecType) {
    return false;
  }

  if (!VTP.hasVFTable(RecType)) {
    return false;
  }
  return getVFTIndex(CallSite) >= 0;
}

namespace psr {

Resolver::Resolver(const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP)
    : IRDB(IRDB), VTP(VTP) {
  assert(IRDB != nullptr);
  assert(VTP != nullptr);
}

const llvm::Function *
Resolver::getNonPureVirtualVFTEntry(const llvm::DIType *T, unsigned Idx,
                                    const llvm::CallBase *CallSite) {
  if (!VTP) {
    return nullptr;
  }

  if (const auto *VT = VTP->getVFTableOrNull(T)) {
    const auto *Target = VT->getFunction(Idx);
    if (Target &&
        Target->getName() != DIBasedTypeHierarchy::PureVirtualCallName &&
        isConsistentCall(CallSite, Target)) {
      return Target;
    }
  }

  return nullptr;
}

void Resolver::preCall(const llvm::Instruction *Inst) {}

void Resolver::handlePossibleTargets(const llvm::CallBase *CallSite,
                                     FunctionSetTy &PossibleTargets) {}

void Resolver::postCall(const llvm::Instruction *Inst) {}

auto Resolver::resolveIndirectCall(const llvm::CallBase *CallSite)
    -> FunctionSetTy {
  if (VTP && isVirtualCall(CallSite, *VTP)) {
    return resolveVirtualCall(CallSite);
  }
  return resolveFunctionPointer(CallSite);
}

auto Resolver::resolveFunctionPointer(const llvm::CallBase *CallSite)
    -> FunctionSetTy {
  // we may wish to optimise this function
  // naive implementation that considers every function whose signature
  // matches the call-site's signature as a callee target
  PHASAR_LOG_LEVEL(DEBUG,
                   "Call function pointer: " << llvmIRToString(CallSite));
  FunctionSetTy CalleeTargets;

  for (const auto *F : IRDB->getAllFunctions()) {
    if (F->hasAddressTaken() && isConsistentCall(CallSite, F)) {
      CalleeTargets.insert(F);
    }
  }

  return CalleeTargets;
}

void Resolver::otherInst(const llvm::Instruction *Inst) {}

std::unique_ptr<Resolver> Resolver::create(CallGraphAnalysisType Ty,
                                           const LLVMProjectIRDB *IRDB,
                                           const LLVMVFTableProvider *VTP,
                                           const DIBasedTypeHierarchy *TH,
                                           LLVMAliasInfoRef PT) {
  assert(IRDB != nullptr);
  assert(VTP != nullptr);

  switch (Ty) {
  case CallGraphAnalysisType::NORESOLVE:
    return std::make_unique<NOResolver>(IRDB, VTP);
  case CallGraphAnalysisType::CHA:
    assert(TH != nullptr);
    return std::make_unique<CHAResolver>(IRDB, VTP, TH);
  case CallGraphAnalysisType::RTA:
    assert(TH != nullptr);
    return std::make_unique<RTAResolver>(IRDB, VTP, TH);
  case CallGraphAnalysisType::DTA:
    assert(TH != nullptr);
    PHASAR_LOG_LEVEL(ERROR, "Do not use the DTA resolver anymore. It relies on "
                            "the removed 'typed-pointers' feature of LLVM.");
    std::exit(1);
    // return std::make_unique<DTAResolver>(IRDB, VTP, TH);
  case CallGraphAnalysisType::VTA:
    llvm::report_fatal_error(
        "The VTA callgraph algorithm is not implemented yet");
  case CallGraphAnalysisType::OTF:
    assert(PT);
    return std::make_unique<OTFResolver>(IRDB, VTP, PT);
  case CallGraphAnalysisType::Invalid:
    llvm::report_fatal_error("Invalid callgraph algorithm specified");
  }

  llvm_unreachable("All possible callgraph algorithms should be handled in the "
                   "above switch");
}

} // namespace psr
