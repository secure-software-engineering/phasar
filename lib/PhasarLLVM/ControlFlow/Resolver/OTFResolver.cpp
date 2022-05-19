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

#include <memory>

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
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

void OTFResolver::preCall(const llvm::Instruction *Inst) {}

void OTFResolver::handlePossibleTargets(const llvm::CallBase *CallSite,
                                        FunctionSetTy &CalleeTargets) {
  // if we have no inter-procedural points-to information, use call-graph
  // information to simulate inter-procedural points-to information
  if (!PT.isInterProcedural()) {
    for (const auto *CalleeTarget : CalleeTargets) {
      PHASAR_LOG_LEVEL(DEBUG, "Target name: " << CalleeTarget->getName());
      // do the merge of the points-to information for all possible targets, but
      // only if they are available
      if (CalleeTarget->isDeclaration()) {
        continue;
      }
      // handle parameter pairs
      for (auto &[Actual, Formal] :
           getActualFormalPointerPairs(CallSite, CalleeTarget)) {
        PT.introduceAlias(Actual, Formal, CallSite);
      }
      // handle return value
      if (CalleeTarget->getReturnType()->isPointerTy()) {
        for (const auto &ExitPoint : ICF.getExitPointsOf(CalleeTarget)) {
          // get the function's return value
          if (const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(ExitPoint)) {
            // introduce alias to the returned value
            PT.introduceAlias(CallSite, Ret->getReturnValue(), CallSite);
          }
        }
      }
    }
  }
}

void OTFResolver::postCall(const llvm::Instruction *Inst) {}

auto OTFResolver::resolveVirtualCall(const llvm::CallBase *CallSite)
    -> FunctionSetTy {
  FunctionSetTy PossibleCallTargets;

  PHASAR_LOG_LEVEL(DEBUG,
                   "Call virtual function: " << llvmIRToString(CallSite));

  auto RetrievedVtableIndex = getVFTIndex(CallSite);
  if (!RetrievedVtableIndex.has_value()) {
    // An error occured
    PHASAR_LOG_LEVEL(DEBUG,
                     "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                         << llvmIRToString(CallSite) << "\n");
    return {};
  }

  auto VtableIndex = RetrievedVtableIndex.value();

  PHASAR_LOG_LEVEL(DEBUG, "Virtual function table entry is: " << VtableIndex);

  const llvm::Value *Receiver = CallSite->getArgOperand(0);

  if (CallSite->getCalledOperand() &&
      CallSite->getCalledOperand()->getType()->isPointerTy()) {
    if (const auto *FTy = llvm::dyn_cast<llvm::FunctionType>(
            CallSite->getCalledOperand()->getType()->getPointerElementType())) {

      auto PTS = PT.getPointsToSet(CallSite->getCalledOperand(), CallSite);
      for (const auto *P : *PTS) {
        if (auto *PGV = llvm::dyn_cast<llvm::GlobalVariable>(P)) {
          if (PGV->hasName() &&
              PGV->getName().startswith(LLVMTypeHierarchy::VTablePrefix) &&
              PGV->hasInitializer()) {
            if (auto *PCS = llvm::dyn_cast<llvm::ConstantStruct>(
                    PGV->getInitializer())) {
              auto VFs = LLVMVFTable::getVFVectorFromIRVTable(*PCS);
              if (VtableIndex >= VFs.size()) {
                continue;
              }
              auto *Callee = VFs[VtableIndex];
              if (Callee == nullptr || !Callee->hasName() ||
                  Callee->getName() == LLVMTypeHierarchy::PureVirtualCallName) {
                continue;
              }
              PossibleCallTargets.insert(Callee);
            }
          }
        }
      }
    }
  }

  return PossibleCallTargets;
}

auto OTFResolver::resolveFunctionPointer(const llvm::CallBase *CallSite)
    -> FunctionSetTy {
  FunctionSetTy Callees;
  if (CallSite->getCalledOperand() &&
      CallSite->getCalledOperand()->getType()->isPointerTy()) {
    if (const auto *FTy = llvm::dyn_cast<llvm::FunctionType>(
            CallSite->getCalledOperand()->getType()->getPointerElementType())) {

      auto PTS = PT.getPointsToSet(CallSite->getCalledOperand(), CallSite);

      llvm::SmallVector<const llvm::GlobalVariable *, 2> GlobalVariableWL;
      llvm::SmallVector<const llvm::ConstantAggregate *> ConstantAggregateWL;
      llvm::SmallPtrSet<const llvm::ConstantAggregate *, 4>
          VisitedConstantAggregates;

      for (const auto *P : *PTS) {
        if (!llvm::isa<llvm::Constant>(P)) {
          continue;
        }

        GlobalVariableWL.clear();
        ConstantAggregateWL.clear();

        if (P->getType()->isPointerTy() &&
            P->getType()->getPointerElementType()->isFunctionTy()) {
          if (const auto *F = llvm::dyn_cast<llvm::Function>(P)) {
            if (matchesSignature(F, FTy, false)) {
              Callees.insert(F);
            }
          }
        }

        if (const auto *GVP = llvm::dyn_cast<llvm::GlobalVariable>(P)) {
          GlobalVariableWL.push_back(GVP);
        } else if (const auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(P)) {
          for (const auto &Op : CE->operands()) {
            if (const auto *GVOp = llvm::dyn_cast<llvm::GlobalVariable>(Op)) {
              GlobalVariableWL.push_back(GVOp);
            }
          }
        }

        if (GlobalVariableWL.empty()) {
          continue;
        }

        for (const auto *GV : GlobalVariableWL) {
          if (!GV->hasInitializer()) {
            continue;
          }
          const auto *InitConst = GV->getInitializer();
          if (const auto *InitConstAggregate =
                  llvm::dyn_cast<llvm::ConstantAggregate>(InitConst)) {
            ConstantAggregateWL.push_back(InitConstAggregate);
          }
        }

        VisitedConstantAggregates.clear();

        while (!ConstantAggregateWL.empty()) {
          const auto *ConstAggregateItem = ConstantAggregateWL.pop_back_val();
          // We may have already processed the item, avoid an infinite loop
          if (!VisitedConstantAggregates.insert(ConstAggregateItem).second) {
            continue;
          }
          for (const auto &Op : ConstAggregateItem->operands()) {
            if (const auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(Op)) {
              if (CE->getType()->isPointerTy() &&
                  CE->getType()->getPointerElementType() == FTy &&
                  CE->isCast()) {
                if (const auto *F =
                        llvm::dyn_cast<llvm::Function>(CE->getOperand(0))) {
                  Callees.insert(F);
                }
              }
            }

            if (const auto *F = llvm::dyn_cast<llvm::Function>(Op)) {
              if (matchesSignature(F, FTy, false)) {
                Callees.insert(F);
              }
            } else if (auto *CA = llvm::dyn_cast<llvm::ConstantAggregate>(Op)) {
              ConstantAggregateWL.push_back(CA);
            } else if (auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(Op)) {
              if (!GV->hasInitializer()) {
                continue;
              }
              if (auto *GVCA = llvm::dyn_cast<llvm::ConstantAggregate>(
                      GV->getInitializer())) {
                ConstantAggregateWL.push_back(GVCA);
              }
            }
          }
        }
      }
    }
  }

  return Callees;
}

std::set<const llvm::Type *>
OTFResolver::getReachableTypes(const LLVMPointsToInfo::PointsToSetTy &Values) {
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
OTFResolver::getActualFormalPointerPairs(const llvm::CallBase *CallSite,
                                         const llvm::Function *CalleeTarget) {
  std::vector<std::pair<const llvm::Value *, const llvm::Value *>> Pairs;
  Pairs.reserve(CallSite->getNumArgOperands());
  // ordinary case

  unsigned Idx = 0;
  for (; Idx < CallSite->getNumArgOperands() && Idx < CalleeTarget->arg_size();
       ++Idx) {
    // only collect pointer typed pairs
    if (CallSite->getArgOperand(Idx)->getType()->isPointerTy() &&
        CalleeTarget->getArg(Idx)->getType()->isPointerTy()) {
      Pairs.emplace_back(CallSite->getArgOperand(Idx),
                         CalleeTarget->getArg(Idx));
    }
  }

  if (CalleeTarget->isVarArg()) {
    // in case of vararg, we can pair-up incoming pointer parameters with the
    // vararg pack of the callee target. the vararg pack will alias
    // (intra-procedurally) with any pointer values loaded from the pack
    const llvm::AllocaInst *VarArgs = nullptr;

    for (const auto &I : llvm::instructions(CalleeTarget)) {
      if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
        if (const auto *AT =
                llvm::dyn_cast<llvm::ArrayType>(Alloca->getAllocatedType())) {
          if (const auto *ST =
                  llvm::dyn_cast<llvm::StructType>(AT->getArrayElementType())) {
            if (ST->hasName() && ST->getName() == "struct.__va_list_tag") {
              VarArgs = Alloca;
              break;
            }
          }
        }
      }
    }

    if (VarArgs) {
      for (; Idx < CallSite->getNumArgOperands(); ++Idx) {
        if (CallSite->getArgOperand(Idx)->getType()->isPointerTy()) {
          Pairs.emplace_back(CallSite->getArgOperand(Idx), VarArgs);
        }
      }
    }
  }
  return Pairs;
}
