/*
 * LLVMBasedBackwardCFG.cpp
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#include "LLVMBasedBackwardCFG.hh"

vector<const llvm::Instruction*> LLVMBasedBackwardCFG::getPredsOf(const llvm::Instruction* u) {
	return {};
}

vector<const llvm::Instruction*> LLVMBasedBackwardCFG::getSuccsOf(const llvm::Instruction* n) {
	return {};
}

set<const llvm::Function*> LLVMBasedBackwardCFG::getCalleesOfCallAt(const llvm::Instruction* n) {
	return {};
}

set<const llvm::Instruction*> LLVMBasedBackwardCFG::getReturnSitesOfCallAt(const llvm::Instruction* n) {
	return {};
}

bool LLVMBasedBackwardCFG::isCallStmt(const llvm::Instruction* stmt) {
	return false;
}

bool LLVMBasedBackwardCFG::isExitStmt(const llvm::Instruction* stmt) {
	return false;
}

bool LLVMBasedBackwardCFG::isStartPoint(const llvm::Instruction* stmt) {
	return false;
}

set<const llvm::Instruction*> LLVMBasedBackwardCFG::allNonCallStartNodes() {
	return {};
}

bool LLVMBasedBackwardCFG::isFallThroughSuccessor(const llvm::Instruction* stmt, const llvm::Instruction* succ) {
	return false;
}

bool LLVMBasedBackwardCFG::isBranchTarget(const llvm::Instruction* stmt, const llvm::Instruction* succ) {
	return false;
}
