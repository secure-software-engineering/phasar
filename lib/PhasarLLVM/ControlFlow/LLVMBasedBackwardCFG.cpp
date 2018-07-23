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

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h>

using namespace psr;
using namespace std;

namespace psr {

const llvm::Function *
LLVMBasedBackwardCFG::getMethodOf(const llvm::Instruction *stmt) {
  return nullptr;
}

vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getPredsOf(const llvm::Instruction *stmt) {
  return {};
}

vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getSuccsOf(const llvm::Instruction *stmt) {
  return {};
}

vector<pair<const llvm::Instruction *, const llvm::Instruction *>>
LLVMBasedBackwardCFG::getAllControlFlowEdges(const llvm::Function *fun) {
  return {};
}

vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getAllInstructionsOf(const llvm::Function *fun) {
  return {};
}

bool LLVMBasedBackwardCFG::isExitStmt(const llvm::Instruction *stmt) {
  return false;
}

bool LLVMBasedBackwardCFG::isStartPoint(const llvm::Instruction *stmt) {
  return false;
}

bool LLVMBasedBackwardCFG::isFallThroughSuccessor(
    const llvm::Instruction *stmt, const llvm::Instruction *succ) {
  return false;
}

bool LLVMBasedBackwardCFG::isBranchTarget(const llvm::Instruction *stmt,
                                          const llvm::Instruction *succ) {
  return false;
}

string LLVMBasedBackwardCFG::getMethodName(const llvm::Function *fun) {
  return "";
}
} // namespace psr
