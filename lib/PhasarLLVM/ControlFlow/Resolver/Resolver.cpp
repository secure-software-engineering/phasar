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

#include <set>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace psr;

namespace psr {

int getVFTIndex(llvm::ImmutableCallSite CS) {
  // deal with a virtual member function
  // retrieve the vtable entry that is called
  const llvm::LoadInst *Load =
      llvm::dyn_cast<llvm::LoadInst>(CS.getCalledValue());
  if (Load == nullptr) {
    return -1;
  }
  const llvm::GetElementPtrInst *GEP =
      llvm::dyn_cast<llvm::GetElementPtrInst>(Load->getPointerOperand());
  if (GEP == nullptr) {
    return -2;
  }
  if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(GEP->getOperand(1))) {
    return CI->getZExtValue();
  }
  return -3;
}

const llvm::StructType *getReceiverType(llvm::ImmutableCallSite CS) {
  if (CS.getNumArgOperands() > 0) {
    const llvm::Value *Receiver = CS.getArgOperand(0);
    if (Receiver->getType()->isPointerTy()) {
      if (const llvm::StructType *ReceiverTy = llvm::dyn_cast<llvm::StructType>(
              Receiver->getType()->getPointerElementType())) {
        return ReceiverTy;
      }
    }
  }
  return nullptr;
}

std::string getReceiverTypeName(llvm::ImmutableCallSite CS) {
  const auto RT = getReceiverType(CS);
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
                                    llvm::ImmutableCallSite CS) {
  if (TH->hasVFTable(T)) {
    auto Target = TH->getVFTable(T)->getFunction(Idx);
    if (Target->getName() != "__cxa_pure_virtual") {
      return Target;
    }
  }
  return nullptr;
}

void Resolver::preCall(const llvm::Instruction *inst) {}

void Resolver::handlePossibleTargets(
    llvm::ImmutableCallSite CS,
    std::set<const llvm::Function *> &possible_targets) {}

void Resolver::postCall(const llvm::Instruction *inst) {}

std::set<const llvm::Function *>
Resolver::resolveFunctionPointer(llvm::ImmutableCallSite CS) {
  // we may wish to optimise this function
  // naive implementation that considers every function whose signature
  // matches the call-site's signature as a callee target
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Call function pointer: "
                << llvmIRToString(CS.getInstruction()));
  std::set<const llvm::Function *> CalleeTargets;
  // *CS.getCalledValue() == nullptr* can happen in extremely rare cases (the
  // origin is still unknown)
  if (CS.getCalledValue() != nullptr &&
      CS.getCalledValue()->getType()->isPointerTy()) {
    if (const llvm::FunctionType *FTy = llvm::dyn_cast<llvm::FunctionType>(
            CS.getCalledValue()->getType()->getPointerElementType())) {
      for (auto F : IRDB.getAllFunctions()) {
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
