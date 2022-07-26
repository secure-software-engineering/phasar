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

#include <optional>
#include <set>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace psr;

namespace psr {

std::optional<unsigned> getVFTIndex(const llvm::CallBase *CallSite) {
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

const llvm::StructType *getReceiverType(const llvm::CallBase *CallSite) {
  if (CallSite->arg_empty() ||
      (CallSite->hasStructRetAttr() && CallSite->arg_size() < 2)) {
    return nullptr;
  }

  const auto *Receiver =
      CallSite->getArgOperand(unsigned(CallSite->hasStructRetAttr()));

  if (!Receiver->getType()->isPointerTy()) {
    return nullptr;
  }

  if (Receiver->getType()->isOpaquePointerTy()) {
    llvm::errs() << "WARNING: The IR under analysis uses opaque pointers, "
                    "which are not supported by phasar yet!\n";
    return nullptr;
  }

  if (const auto *ReceiverTy = llvm::dyn_cast<llvm::StructType>(
          Receiver->getType()->getPointerElementType())) {
    return ReceiverTy;
  }

  return nullptr;
}

std::string getReceiverTypeName(const llvm::CallBase *CallSite) {
  const auto *RT = getReceiverType(CallSite);
  if (RT) {
    return RT->getName().str();
  }
  return "";
}

Resolver::Resolver(ProjectIRDB &IRDB) : IRDB(IRDB), TH(nullptr) {}

Resolver::Resolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH)
    : IRDB(IRDB), TH(&TH) {}

const llvm::Function *
Resolver::getNonPureVirtualVFTEntry(const llvm::StructType *T, unsigned Idx,
                                    const llvm::CallBase * /*CallSite*/) {
  if (TH && TH->hasVFTable(T)) {
    const auto *Target = TH->getVFTable(T)->getFunction(Idx);
    if (Target && Target->getName() != "__cxa_pure_virtual") {
      return Target;
    }
  }
  return nullptr;
}

void Resolver::preCall(const llvm::Instruction *Inst) {}

void Resolver::handlePossibleTargets(const llvm::CallBase *CallSite,
                                     FunctionSetTy &PossibleTargets) {}

void Resolver::postCall(const llvm::Instruction *Inst) {}

auto Resolver::resolveFunctionPointer(const llvm::CallBase *CallSite)
    -> FunctionSetTy {
  // we may wish to optimise this function
  // naive implementation that considers every function whose signature
  // matches the call-site's signature as a callee target
  PHASAR_LOG_LEVEL(DEBUG,
                   "Call function pointer: " << llvmIRToString(CallSite));
  FunctionSetTy CalleeTargets;
  // *CS.getCalledValue() == nullptr* can happen in extremely rare cases (the
  // origin is still unknown)
  if (CallSite->getCalledOperand() != nullptr &&
      CallSite->getCalledOperand()->getType()->isPointerTy()) {
    if (const auto *FTy = llvm::dyn_cast<llvm::FunctionType>(
            CallSite->getCalledOperand()->getType()->getPointerElementType())) {
      for (const auto *F : IRDB.getAllFunctions()) {
        if (matchesSignature(F, FTy)) {
          CalleeTargets.insert(F);
        }
      }
    }
  }
  return CalleeTargets;
}

void Resolver::otherInst(const llvm::Instruction *Inst) {}

} // namespace psr
