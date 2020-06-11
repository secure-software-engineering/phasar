/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMBasedCFG.cpp
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/Utils/LLVMShorthands.h"

#include <algorithm>
#include <cassert>
#include <iterator>

using namespace std;
using namespace psr;

namespace psr {

const llvm::Function *
LLVMBasedCFG::getFunctionOf(const llvm::Instruction *Stmt) const {
  return Stmt->getFunction();
}

vector<const llvm::Instruction *>
LLVMBasedCFG::getPredsOf(const llvm::Instruction *I) const {
  vector<const llvm::Instruction *> Preds;
  if (I->getPrevNode()) {
    Preds.push_back(I->getPrevNode());
  }
  /*
   * If we do not have a predecessor yet, look for basic blocks which
   * lead to our instruction in question!
   */
  if (Preds.empty()) {
    std::transform(llvm::pred_begin(I->getParent()),
                   llvm::pred_end(I->getParent()), back_inserter(Preds),
                   [](const llvm::BasicBlock *BB) {
                     assert(BB && "BB under analysis was not well formed.");
                     return BB->getTerminator();
                   });
  }
  return Preds;
}

vector<const llvm::Instruction *>
LLVMBasedCFG::getSuccsOf(const llvm::Instruction *I) const {
  vector<const llvm::Instruction *> Successors;
  if (I->getNextNode()) {
    Successors.push_back(I->getNextNode());
  }
  if (I->isTerminator()) {
    Successors.reserve(I->getNumSuccessors() + Successors.size());
    std::transform(llvm::succ_begin(I), llvm::succ_end(I),
                   back_inserter(Successors),
                   [](const llvm::BasicBlock *BB) { return &BB->front(); });
  }
  return Successors;
} // namespace psr

vector<pair<const llvm::Instruction *, const llvm::Instruction *>>
LLVMBasedCFG::getAllControlFlowEdges(const llvm::Function *Fun) const {
  vector<pair<const llvm::Instruction *, const llvm::Instruction *>> Edges;
  for (const auto &BB : *Fun) {
    for (const auto &I : BB) {
      auto Successors = getSuccsOf(&I);
      for (const auto *Successor : Successors) {
        Edges.emplace_back(&I, Successor);
      }
    }
  }
  return Edges;
}

vector<const llvm::Instruction *>
LLVMBasedCFG::getAllInstructionsOf(const llvm::Function *Fun) const {
  vector<const llvm::Instruction *> Instructions;
  for (const auto &BB : *Fun) {
    for (const auto &I : BB) {
      Instructions.push_back(&I);
    }
  }
  return Instructions;
}

bool LLVMBasedCFG::isExitStmt(const llvm::Instruction *Stmt) const {
  return llvm::isa<llvm::ReturnInst>(Stmt);
}

bool LLVMBasedCFG::isStartPoint(const llvm::Instruction *Stmt) const {
  return (Stmt == &Stmt->getFunction()->front().front());
}

bool LLVMBasedCFG::isFieldLoad(const llvm::Instruction *Stmt) const {
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Stmt)) {
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Load->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

bool LLVMBasedCFG::isFieldStore(const llvm::Instruction *Stmt) const {
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Stmt)) {
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Store->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

bool LLVMBasedCFG::isFallThroughSuccessor(const llvm::Instruction *Stmt,
                                          const llvm::Instruction *Succ) const {
  // assert(false && "FallThrough not valid in LLVM IR");
  if (const auto *B = llvm::dyn_cast<llvm::BranchInst>(Stmt)) {
    if (B->isConditional()) {
      return &B->getSuccessor(1)->front() == Succ;
    } else {
      return &B->getSuccessor(0)->front() == Succ;
    }
  }
  return false;
}

bool LLVMBasedCFG::isBranchTarget(const llvm::Instruction *Stmt,
                                  const llvm::Instruction *Succ) const {
  if (Stmt->isTerminator()) {
    for (const auto *BB : llvm::successors(Stmt->getParent())) {
      if (&BB->front() == Succ) {
        return true;
      }
    }
  }
  return false;
}

string LLVMBasedCFG::getStatementId(const llvm::Instruction *Stmt) const {
  return llvm::cast<llvm::MDString>(
             Stmt->getMetadata(PhasarConfig::MetaDataKind())->getOperand(0))
      ->getString()
      .str();
}

string LLVMBasedCFG::getFunctionName(const llvm::Function *Fun) const {
  return Fun->getName().str();
}

void LLVMBasedCFG::print(const llvm::Function *F, std::ostream &OS) const {
  OS << llvmIRToString(F);
}

nlohmann::json LLVMBasedCFG::getAsJson(const llvm::Function *F) const {
  return "";
}

} // namespace psr
