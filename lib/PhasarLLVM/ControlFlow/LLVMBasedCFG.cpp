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

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace std;
using namespace psr;

namespace psr {

const llvm::Function *
LLVMBasedCFG::getFunctionOf(const llvm::Instruction *stmt) const {
  return stmt->getFunction();
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
    for (auto &BB : *I->getFunction()) {
      if (const llvm::Instruction *T = BB.getTerminator()) {
        for (unsigned i = 0; i < T->getNumSuccessors(); ++i) {
          if (&*T->getSuccessor(i)->begin() == I) {
            Preds.push_back(T);
          }
        }
      }
    }
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
    for (unsigned i = 0; i < I->getNumSuccessors(); ++i) {
      Successors.push_back(&*I->getSuccessor(i)->begin());
    }
  }
  return Successors;
}

vector<pair<const llvm::Instruction *, const llvm::Instruction *>>
LLVMBasedCFG::getAllControlFlowEdges(const llvm::Function *fun) const {
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
LLVMBasedCFG::getAllInstructionsOf(const llvm::Function *fun) const {
  vector<const llvm::Instruction *> Instructions;
  for (auto &BB : *fun) {
    for (auto &I : BB) {
      Instructions.push_back(&I);
    }
  }
  return Instructions;
}

bool LLVMBasedCFG::isExitStmt(const llvm::Instruction *stmt) const {
  return llvm::isa<llvm::ReturnInst>(stmt);
}

bool LLVMBasedCFG::isStartPoint(const llvm::Instruction *stmt) const {
  return (stmt == &stmt->getFunction()->front().front());
}

bool LLVMBasedCFG::isFieldLoad(const llvm::Instruction *stmt) const {
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(stmt)) {
    if (auto GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Load->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

bool LLVMBasedCFG::isFieldStore(const llvm::Instruction *stmt) const {
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(stmt)) {
    if (auto GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
            Store->getPointerOperand())) {
      return true;
    }
  }
  return false;
}

bool LLVMBasedCFG::isFallThroughSuccessor(const llvm::Instruction *stmt,
                                          const llvm::Instruction *succ) const {
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
                                  const llvm::Instruction *succ) const {
  if (stmt->isTerminator()) {
    for (unsigned i = 0; i < stmt->getNumSuccessors(); ++i) {
      if (&*stmt->getSuccessor(i)->begin() == succ) {
        return true;
      }
    }
  }
  return false;
}

string LLVMBasedCFG::getStatementId(const llvm::Instruction *stmt) const {
  return llvm::cast<llvm::MDString>(
             stmt->getMetadata(PhasarConfig::MetaDataKind())->getOperand(0))
      ->getString()
      .str();
}

string LLVMBasedCFG::getFunctionName(const llvm::Function *fun) const {
  return fun->getName().str();
}

void LLVMBasedCFG::print(const llvm::Function *F, std::ostream &OS) const {
  OS << llvmIRToString(F);
}

nlohmann::json LLVMBasedCFG::getAsJson(const llvm::Function *F) const {
  return "";
}

} // namespace psr
