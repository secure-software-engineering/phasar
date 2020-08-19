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
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
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
                         LLVMBasedICFG &ICF, LLVMPointsToInfo &PT)
    : CHAResolver(IRDB, TH), ICF(ICF), PT(PT) {}

void OTFResolver::preCall(const llvm::Instruction *Inst) {
  CallStack.push_back(Inst);
}

void OTFResolver::handlePossibleTargets(
    llvm::ImmutableCallSite CS,
    std::set<const llvm::Function *> &CalleeTargets) {
  // if we have no inter-procedural points-to information, use call-graph
  // information to simulate inter-procedural points-to information
  if (!PT.isInterProcedural()) {
    for (const auto *CalleeTarget : CalleeTargets) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Target name: " << CalleeTarget->getName().str());
      // do the merge of the points-to information for all possible targets, but
      // only if they are available
      if (!CalleeTarget->isDeclaration()) {
        // handle parameter pairs
        auto Pairs = getActualFormalPointerPairs(CS, CalleeTarget);
        for (auto &[Actual, Formal] : Pairs) {
          PT.introduceAlias(Actual, Formal, CS.getInstruction());
        }
        // handle return value
        if (CalleeTarget->getReturnType()->isPointerTy()) {
          for (const auto &ExitPoint : ICF.getExitPointsOf(CalleeTarget)) {
            // get the function's return value
            if (const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(ExitPoint)) {
              // introduce alias to the returned value
              PT.introduceAlias(CS.getInstruction(), Ret->getReturnValue(),
                                CS.getInstruction());
            }
          }
        }
      }
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

  // Use points-to information to resolve the indirect call
  auto AllocSites = PT.getReachableAllocationSites(Receiver);
  auto PossibleAllocatedTypes = getReachableTypes(AllocSites);

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
  const auto PTS = PT.getPointsToSet(CS.getCalledValue());
  for (const auto *P : *PTS) {
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

std::set<const llvm::Type *> OTFResolver::getReachableTypes(
    const std::unordered_set<const llvm::Value *> &Values) {
  std::set<const llvm::Type *> Types;
  // an allocation site can either be an AllocaInst or a call to an
  // allocating function
  for (const auto *V : Values) {
    if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(V)) {
      Types.insert(Alloc->getAllocatedType());
    } else {
      // usually if an allocating function is called, it is immediately
      // bit-casted
      // to the desired allocated value and hence we can determine it from
      // the destination type of that cast instruction.
      for (const auto *User : V->users()) {
        if (const auto *Cast = llvm::dyn_cast<llvm::BitCastInst>(User)) {
          Types.insert(Cast->getDestTy());
        }
      }
    }
  }
  return Types;
}

std::vector<std::pair<const llvm::Value *, const llvm::Value *>>
OTFResolver::getActualFormalPointerPairs(llvm::ImmutableCallSite CS,
                                         const llvm::Function *CalleeTarget) {
  std::vector<std::pair<const llvm::Value *, const llvm::Value *>> Pairs;
  // ordinary case
  if (!CalleeTarget->isVarArg()) {
    Pairs.reserve(CS.arg_size());
    for (unsigned Idx = 0;
         Idx < CS.arg_size() && Idx < CalleeTarget->arg_size(); ++Idx) {
      // only collect pointer typed pairs
      if (CS.getArgOperand(Idx)->getType()->isPointerTy() &&
          CalleeTarget->getArg(Idx)->getType()->isPointerTy()) {
        Pairs.emplace_back(CS.getArgOperand(Idx), CalleeTarget->getArg(Idx));
      }
    }
  } else {
    // in case of vararg, we can pair-up incoming pointer parameters with the
    // vararg pack of the callee target. the vararg pack will alias
    // (intra-procedurally) with any pointer values loaded from the pack
    const llvm::AllocaInst *VarArgs = nullptr;
    for (const auto &BB : *CalleeTarget) {
      for (const auto &I : BB) {
        if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
          if (const auto *AT =
                  llvm::dyn_cast<llvm::ArrayType>(Alloca->getAllocatedType())) {
            if (const auto *ST = llvm::dyn_cast<llvm::StructType>(
                    AT->getArrayElementType())) {
              if (ST->hasName() && ST->getName() == "struct.__va_list_tag") {
                VarArgs = Alloca;
                break;
              }
            }
          }
        }
      }
    }
    if (VarArgs) {
      for (unsigned Idx = 0; Idx < CS.arg_size(); ++Idx) {
        if (CS.getArgOperand(Idx)->getType()->isPointerTy()) {
          Pairs.emplace_back(CS.getArgOperand(Idx), VarArgs);
        }
      }
    }
  }
  return Pairs;
}
