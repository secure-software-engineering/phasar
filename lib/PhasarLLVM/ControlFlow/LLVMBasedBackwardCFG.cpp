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

// same as LLVMBasedCFG
const llvm::Function *
LLVMBasedBackwardCFG::getFunctionOf(const llvm::Instruction *Stmt) const {
  return Stmt->getParent()->getParent();
}

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

std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
LLVMBasedBackwardCFG::getAllControlFlowEdges(const llvm::Function *Fun) const {
  vector<pair<const llvm::Instruction *, const llvm::Instruction *>> Edges;
  for (const auto &BB : *Fun) {
    for (const auto &I : BB) {
      auto Successors = getSuccsOf(&I);
      for (const auto *Successor : Successors) {
        Edges.insert(Edges.begin(), make_pair(Successor, &I));
      }
    }
  }
  return Edges;
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getAllInstructionsOf(const llvm::Function *Fun) const {
  vector<const llvm::Instruction *> Instructions;
  for (const auto &BB : *Fun) {
    for (const auto &I : BB) {
      Instructions.insert(Instructions.begin(), &I);
    }
  }
  return Instructions;
}

// LLVMBasedCFG::isStartPoint
bool LLVMBasedBackwardCFG::isExitStmt(const llvm::Instruction *Stmt) const {
  return (Stmt == &Stmt->getFunction()->front().front());
}

// LLVMBasedCFG::isExitStmt
bool LLVMBasedBackwardCFG::isStartPoint(const llvm::Instruction *Stmt) const {
  return llvm::isa<llvm::ReturnInst>(Stmt);
}

bool LLVMBasedBackwardCFG::isFieldLoad(const llvm::Instruction *Stmt) const {
  return ForwardCFG.isFieldLoad(Stmt);
}

bool LLVMBasedBackwardCFG::isFieldStore(const llvm::Instruction *Stmt) const {
  return ForwardCFG.isFieldStore(Stmt);
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

// same as LLVMBasedCFG
std::string
LLVMBasedBackwardCFG::getFunctionName(const llvm::Function *Fun) const {
  return Fun->getName().str();
}

std::string
LLVMBasedBackwardCFG::getStatementId(const llvm::Instruction *Stmt) const {
  return llvm::cast<llvm::MDString>(
             Stmt->getMetadata(PhasarConfig::MetaDataKind())->getOperand(0))
      ->getString()
      .str();
}

void LLVMBasedBackwardCFG::print(const llvm::Function *F,
                                 std::ostream &OS) const {
  OS << llvmIRToString(F);
}

nlohmann::json LLVMBasedBackwardCFG::getAsJson(const llvm::Function *F) const {
  return "";
}

} // namespace psr
