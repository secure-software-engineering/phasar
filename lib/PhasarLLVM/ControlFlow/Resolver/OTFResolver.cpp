/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 *  DTAResolver.cpp
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/OTFResolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;

OTFResolver::OTFResolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH,
                         LLVMPointsToInfo &PT,
                         LLVMPointsToGraph &WholeModulePTG)
    : CHAResolver(IRDB, TH), PT(PT), WholeModulePTG(WholeModulePTG) {}

void OTFResolver::preCall(const llvm::Instruction *Inst) {
  CallStack.push_back(Inst);
}

void OTFResolver::handlePossibleTargets(
    llvm::ImmutableCallSite CS,
    std::set<const llvm::Function *> &CalleeTargets) {

  for (const auto *CalleeTarget : CalleeTargets) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Target name: " << CalleeTarget->getName().str());
    // Do the merge of the points-to graphs for all possible targets, but
    // only if they are available
    if (!CalleeTarget->isDeclaration()) {
      auto *CalleePTG = PT.getPointsToGraph(CalleeTarget);
      WholeModulePTG.mergeWith(CalleePTG, CalleeTarget);
      WholeModulePTG.mergeCallSite(CS, CalleeTarget);
    }
  }
}

void OTFResolver::postCall(const llvm::Instruction *Inst) {
  CallStack.pop_back();
}

set<const llvm::Function *>
OTFResolver::resolveVirtualCall(llvm::ImmutableCallSite CS) {
  set<const llvm::Function *> PossibleCallTargets;

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "Call virtual function: "
                << llvmIRToString(CS.getInstruction()));

  auto VtableIndex = getVFTIndex(CS);
  if (VtableIndex < 0) {
    // An error occured
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                  << llvmIRToString(CS.getInstruction()) << "\n");
    return {};
  }

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "Virtual function table entry is: " << VtableIndex);

  const llvm::Value *Receiver = CS.getArgOperand(0);

  auto AllocSites =
      WholeModulePTG.getReachableAllocationSites(Receiver, CallStack);
  auto PossibleAllocatedTypes =
      psr::LLVMPointsToGraph::computeTypesFromAllocationSites(AllocSites);

  const auto *ReceiverType = getReceiverType(CS);

  // Now we must check if we have found some allocated struct types
  set<const llvm::StructType *> PossibleTypes;
  for (const auto *Type : PossibleAllocatedTypes) {
    if (const auto *StructType =
            llvm::dyn_cast<llvm::StructType>(stripPointer(Type))) {
      PossibleTypes.insert(StructType);
    }
  }

  for (const auto *PossibleTypeStruct : PossibleTypes) {
    const auto *Target =
        getNonPureVirtualVFTEntry(PossibleTypeStruct, VtableIndex, CS);
    if (Target) {
      PossibleCallTargets.insert(Target);
    }
  }
  if (PossibleCallTargets.empty()) {
    return CHAResolver::resolveVirtualCall(CS);
  }

  return PossibleCallTargets;
}

std::set<const llvm::Function *>
OTFResolver::resolveFunctionPointer(llvm::ImmutableCallSite CS) {
  std::set<const llvm::Function *> Callees;
  auto PTS = PT.getPointsToSet(CS.getCalledValue());
  for (const auto *P : PTS) {
    if (P->getType()->isPointerTy() &&
        P->getType()->getPointerElementType()->isFunctionTy()) {
      if (const auto *F = llvm::dyn_cast<llvm::Function>(P)) {
        Callees.insert(F);
      }
    }
  }
  // if we could not find any callees, use a fallback solution
  if (Callees.empty()) {
    return Resolver::resolveFunctionPointer(CS);
  }
  return Callees;
}
