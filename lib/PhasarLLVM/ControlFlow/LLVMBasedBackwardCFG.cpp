/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMBasedBackwardCFG.cpp
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace psr;
using namespace std;

namespace psr {
// TODO: isFallTroughtSuccessor, isBranchTarget

std::vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getPredsOf(const llvm::Instruction *Inst) const {
  vector<const llvm::Instruction *> Preds;
  if (Inst->getNextNode()) {
    Preds.push_back(Inst->getNextNode());
  }
  if (Inst->isTerminator()) {
    for (unsigned I = 0; I < Inst->getNumSuccessors(); ++I) {
      Preds.push_back(&*Inst->getSuccessor(I)->begin());
    }
  }
  return Preds;
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getSuccsOf(const llvm::Instruction *Inst) const {
  vector<const llvm::Instruction *> Preds;
  if (Inst->getPrevNode()) {
    Preds.push_back(Inst->getPrevNode());
  }
  if (Inst == &Inst->getParent()->front()) {
    for (const auto *PredBlock : llvm::predecessors(Inst->getParent())) {
      Preds.push_back(&PredBlock->back());
    }
  }
  return Preds;
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardCFG::getStartPointsOf(const llvm::Function *Fun) const {
  return LLVMBasedCFG::getExitPointsOf(Fun);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardCFG::getExitPointsOf(const llvm::Function *Fun) const {
  return LLVMBasedCFG::getStartPointsOf(Fun);
}

// LLVMBasedCFG::isStartPoint
bool LLVMBasedBackwardCFG::isExitInst(const llvm::Instruction *Inst) const {
  return (Inst == &Inst->getFunction()->front().front());
}

// LLVMBasedCFG::isExitInst
bool LLVMBasedBackwardCFG::isStartPoint(const llvm::Instruction *Inst) const {
  return llvm::isa<llvm::ReturnInst>(Inst);
}

bool LLVMBasedBackwardCFG::isFallThroughSuccessor(
    const llvm::Instruction * /*Inst*/,
    const llvm::Instruction * /*Succ*/) const {
  assert(false && "FallThrough not valid in LLVM IR");
  return false;
}

bool LLVMBasedBackwardCFG::isBranchTarget(const llvm::Instruction *Inst,
                                          const llvm::Instruction *Succ) const {
  if (const auto *B = llvm::dyn_cast<llvm::BranchInst>(Succ)) {
    for (const auto *BB : B->successors()) {
      if (Inst == &(BB->front())) {
        return true;
      }
    }
  }
  return false;
}

} // namespace psr
