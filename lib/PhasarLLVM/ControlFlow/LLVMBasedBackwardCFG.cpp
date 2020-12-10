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
LLVMBasedBackwardCFG::getPredsOf(const llvm::Instruction *Stmt) const {
  vector<const llvm::Instruction *> Preds;
  if (Stmt->getNextNode()) {
    Preds.push_back(Stmt->getNextNode());
  }
  if (Stmt->isTerminator()) {
    for (unsigned I = 0; I < Stmt->getNumSuccessors(); ++I) {
      Preds.push_back(&*Stmt->getSuccessor(I)->begin());
    }
  }
  return Preds;
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getSuccsOf(const llvm::Instruction *Stmt) const {
  vector<const llvm::Instruction *> Preds;
  if (Stmt->getPrevNode()) {
    Preds.push_back(Stmt->getPrevNode());
  }
  if (Stmt == &Stmt->getParent()->front()) {
    for (const auto *PredBlock : llvm::predecessors(Stmt->getParent())) {
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
bool LLVMBasedBackwardCFG::isExitStmt(const llvm::Instruction *Stmt) const {
  return (Stmt == &Stmt->getFunction()->front().front());
}

// LLVMBasedCFG::isExitStmt
bool LLVMBasedBackwardCFG::isStartPoint(const llvm::Instruction *Stmt) const {
  return llvm::isa<llvm::ReturnInst>(Stmt);
}

bool LLVMBasedBackwardCFG::isFallThroughSuccessor(
    const llvm::Instruction *Stmt, const llvm::Instruction *Succ) const {
  assert(false && "FallThrough not valid in LLVM IR");
  return false;
}

bool LLVMBasedBackwardCFG::isBranchTarget(const llvm::Instruction *Stmt,
                                          const llvm::Instruction *Succ) const {
  if (const auto *B = llvm::dyn_cast<llvm::BranchInst>(Succ)) {
    for (const auto *BB : B->successors()) {
      if (Stmt == &(BB->front())) {
        return true;
      }
    }
  }
  return false;
}

} // namespace psr
