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

#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include <phasar/Config/Configuration.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h>

using namespace psr;
using namespace std;

namespace psr {
// TODO: isFallTroughtSuccessor, isBranchTarget

// same as LLVMBasedCFG
const llvm::Function *
LLVMBasedBackwardCFG::getMethodOf(const llvm::Instruction *stmt) {
  return stmt->getParent()->getParent();
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getPredsOf(const llvm::Instruction *stmt) {
  vector<const llvm::Instruction *> preds;
  if (stmt->getNextNode())
    preds.push_back(stmt->getNextNode());
  if (stmt->isTerminator()) {
    for (unsigned i = 0; i < stmt->getNumSuccessors(); ++i) {
      preds.push_back(&*stmt->getSuccessor(i)->begin());
    }
  }
  return preds;
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getSuccsOf(const llvm::Instruction *stmt) {
  vector<const llvm::Instruction *> Preds;
  if (stmt->getPrevNode()) {
    Preds.push_back(stmt->getPrevNode());
  }
  for (auto PredBlock : llvm::predecessors(stmt->getParent())) {
    Preds.push_back(&PredBlock->back());
  }
  return Preds;
}

std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
LLVMBasedBackwardCFG::getAllControlFlowEdges(const llvm::Function *fun) {
  vector<pair<const llvm::Instruction *, const llvm::Instruction *>> Edges;
  for (auto &BB : *fun) {
    for (auto &I : BB) {
      auto Successors = getSuccsOf(&I);
      for (auto Successor : Successors) {
        Edges.insert(Edges.begin(), make_pair(Successor, &I));
      }
    }
  }
  return Edges;
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getAllInstructionsOf(const llvm::Function *fun) {
  vector<const llvm::Instruction *> Instructions;
  for (auto &BB : *fun) {
    for (auto &I : BB) {
      Instructions.insert(Instructions.begin(), &I);
    }
  }
  return Instructions;
}

// LLVMBasedCFG::isStartPoint
bool LLVMBasedBackwardCFG::isExitStmt(const llvm::Instruction *stmt) {
  return (stmt == &stmt->getFunction()->front().front());
}

// LLVMBasedCFG::isExitStmt
bool LLVMBasedBackwardCFG::isStartPoint(const llvm::Instruction *stmt) {
  return llvm::isa<llvm::ReturnInst>(stmt);
}

bool LLVMBasedBackwardCFG::isFieldLoad(const llvm::Instruction *stmt) {
  return ForwardCFG.isFieldLoad(stmt);
}

bool LLVMBasedBackwardCFG::isFieldStore(const llvm::Instruction *stmt) {
  return ForwardCFG.isFieldStore(stmt);
}

bool LLVMBasedBackwardCFG::isFallThroughSuccessor(
    const llvm::Instruction *stmt, const llvm::Instruction *succ) {
  assert(false && "FallThrough not valid in LLVM IR");
  return false;
}

bool LLVMBasedBackwardCFG::isBranchTarget(const llvm::Instruction *stmt,
                                          const llvm::Instruction *succ) {
  if (const llvm::BranchInst *B = llvm::dyn_cast<llvm::BranchInst>(succ)) {
    for (auto BB : B->successors()) {
      if (stmt == &(BB->front())) {
        return true;
      }
    }
  }
  return false;
}

// same as LLVMBasedCFG
std::string LLVMBasedBackwardCFG::getMethodName(const llvm::Function *fun) {
  return fun->getName().str();
}

std::string
LLVMBasedBackwardCFG::getStatementId(const llvm::Instruction *stmt) {
  return llvm::cast<llvm::MDString>(
             stmt->getMetadata(PhasarConfig::MetaDataKind())->getOperand(0))
      ->getString()
      .str();
}
} // namespace psr
