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

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include <phasar/Config/Configuration.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>

using namespace std;
using namespace psr;

namespace psr {

const llvm::Function *LLVMBasedCFG::getMethodOf(const llvm::Instruction *stmt) {
  return stmt->getFunction();
}

vector<const llvm::Instruction *>
LLVMBasedCFG::getPredsOf(const llvm::Instruction *I) {
  vector<const llvm::Instruction *> Preds;
  if (I->getPrevNode()) {
    Preds.push_back(I->getPrevNode());
  }
  /*
   * If we do not have a predecessor yet, look for basic blocks which
   * lead to our instruction in question!
   */
  if (Preds.empty()) {
    for (auto &BB : *I->getFunction()) {
      if (const llvm::TerminatorInst *T =
              llvm::dyn_cast<llvm::TerminatorInst>(BB.getTerminator())) {
        for (auto successor : T->successors()) {
          if (&*successor->begin() == I) {
            Preds.push_back(T);
          }
        }
      }
    }
  }
  return Preds;
}

vector<const llvm::Instruction *>
LLVMBasedCFG::getSuccsOf(const llvm::Instruction *I) {
  vector<const llvm::Instruction *> Successors;
  if (I->getNextNode()) {
    Successors.push_back(I->getNextNode());
  }
  if (const llvm::TerminatorInst *T = llvm::dyn_cast<llvm::TerminatorInst>(I)) {
    for (auto successor : T->successors()) {
      Successors.push_back(&*successor->begin());
    }
  }
  return Successors;
}

vector<pair<const llvm::Instruction *, const llvm::Instruction *>>
LLVMBasedCFG::getAllControlFlowEdges(const llvm::Function *fun) {
  vector<pair<const llvm::Instruction *, const llvm::Instruction *>> Edges;
  for (auto &BB : *fun) {
    for (auto &I : BB) {
      auto Successors = getSuccsOf(&I);
      for (auto Successor : Successors) {
        Edges.push_back(make_pair(&I, Successor));
      }
    }
  }
  return Edges;
}

vector<const llvm::Instruction *>
LLVMBasedCFG::getAllInstructionsOf(const llvm::Function *fun) {
  vector<const llvm::Instruction *> Instructions;
  for (auto &BB : *fun) {
    for (auto &I : BB) {
      Instructions.push_back(&I);
    }
  }
  return Instructions;
}

bool LLVMBasedCFG::isExitStmt(const llvm::Instruction *stmt) {
  return llvm::isa<llvm::ReturnInst>(stmt);
}

bool LLVMBasedCFG::isStartPoint(const llvm::Instruction *stmt) {
  return (stmt == &stmt->getFunction()->front().front());
}

bool LLVMBasedCFG::isFieldLoad(const llvm::Instruction *stmt) {
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(stmt)) {
    if (auto GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Load->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

bool LLVMBasedCFG::isFieldStore(const llvm::Instruction *stmt) {
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(stmt)) {
    if (auto GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Store->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

bool LLVMBasedCFG::isFallThroughSuccessor(const llvm::Instruction *stmt,
                                          const llvm::Instruction *succ) {
  // assert(false && "FallThrough not valid in LLVM IR");
  if (const llvm::BranchInst *B = llvm::dyn_cast<llvm::BranchInst>(stmt)) {
    if (B->isConditional()) {
      return &B->getSuccessor(1)->front() == succ;
    } else {
      return &B->getSuccessor(0)->front() == succ;
    }
  }
  return false;
}

bool LLVMBasedCFG::isBranchTarget(const llvm::Instruction *stmt,
                                  const llvm::Instruction *succ) {
  if (const llvm::TerminatorInst *T =
          llvm::dyn_cast<llvm::TerminatorInst>(stmt)) {
    for (auto successor : T->successors()) {
      if (&*successor->begin() == succ) {
        return true;
      }
    }
  }
  return false;
}

string LLVMBasedCFG::getStatementId(const llvm::Instruction *stmt) {
  return llvm::cast<llvm::MDString>(
             stmt->getMetadata(MetaDataKind)->getOperand(0))
      ->getString()
      .str();
}

string LLVMBasedCFG::getMethodName(const llvm::Function *fun) {
  return fun->getName().str();
}
} // namespace psr
