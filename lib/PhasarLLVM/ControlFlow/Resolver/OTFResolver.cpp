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

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/OTFResolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Utilities.h>

using namespace std;
using namespace psr;

OTFResolver::OTFResolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH,
                         LLVMPointsToInfo &PT, PointsToGraph &WholeModulePTG)
    : CHAResolver(IRDB, TH), PT(PT), WholeModulePTG(WholeModulePTG) {}

void OTFResolver::preCall(const llvm::Instruction *Inst) {
  CallStack.push_back(Inst);
}

void OTFResolver::handlePossibleTargets(
    llvm::ImmutableCallSite CS,
    std::set<const llvm::Function *> &CalleeTargets) {
  auto &lg = lg::get();

  for (auto CalleeTarget : CalleeTargets) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Target name: " << CalleeTarget->getName().str());
    // Do the merge of the points-to graphs for all possible targets, but
    // only if they are available
    if (!CalleeTarget->isDeclaration()) {
      auto CalleePTG = PT.getPointsToGraph(CalleeTarget);
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
  set<const llvm::Function *> possible_call_targets;
  auto &lg = lg::get();

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Call virtual function: "
                << llvmIRToString(CS.getInstruction()));

  auto vtable_index = getVFTIndex(CS);
  if (vtable_index < 0) {
    // An error occured
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                  << llvmIRToString(CS.getInstruction()) << "\n");
    return {};
  }

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Virtual function table entry is: " << vtable_index);

  const llvm::Value *receiver = CS.getArgOperand(0);

  auto alloc_sites =
      WholeModulePTG.getReachableAllocationSites(receiver, CallStack);
  auto possible_allocated_types =
      WholeModulePTG.computeTypesFromAllocationSites(alloc_sites);

  auto receiver_type = getReceiverType(CS);

  // Now we must check if we have found some allocated struct types
  set<const llvm::StructType *> possible_types;
  for (auto type : possible_allocated_types) {
    if (auto struct_type =
            llvm::dyn_cast<llvm::StructType>(stripPointer(type))) {
      possible_types.insert(struct_type);
    }
  }

  for (auto possible_type_struct : possible_types) {
    auto Target =
        getNonPureVirtualVFTEntry(possible_type_struct, vtable_index, CS);
    if (Target) {
      possible_call_targets.insert(Target);
    }
  }
  if (possible_call_targets.empty())
    return CHAResolver::resolveVirtualCall(CS);

  return possible_call_targets;
}

std::set<const llvm::Function *>
OTFResolver::resolveFunctionPointer(llvm::ImmutableCallSite CS) {
  std::set<const llvm::Function *> Callees;
  auto PTS = PT.getPointsToSet(CS.getCalledValue());
  for (auto P : PTS) {
    if (P->getType()->isPointerTy() &&
        P->getType()->getPointerElementType()->isFunctionTy()) {
      if (auto F = llvm::dyn_cast<llvm::Function>(P)) {
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
