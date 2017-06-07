/*
 * LLVMBasedCFG.cpp
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#include "LLVMBasedCFG.hh"


vector<const llvm::Instruction*> LLVMBasedCFG::getPredsOf(const llvm::Instruction* u) {
	return {};
}

vector<const llvm::Instruction*> LLVMBasedCFG::getSuccsOf(const llvm::Instruction* n) {
	return {};
}

set<const llvm::Function*> LLVMBasedCFG::getCalleesOfCallAt(const llvm::Instruction* n) {
	return {};
}

set<const llvm::Instruction*> LLVMBasedCFG::getReturnSitesOfCallAt(const llvm::Instruction* n) {
	return {};
}

bool LLVMBasedCFG::isCallStmt(const llvm::Instruction* stmt) {
	return false;
}

bool LLVMBasedCFG::isExitStmt(const llvm::Instruction* stmt) {
	return false;
}

bool LLVMBasedCFG::isStartPoint(const llvm::Instruction* stmt) {
	return false;
}

set<const llvm::Instruction*> LLVMBasedCFG::allNonCallStartNodes() {
	return {};
}

bool LLVMBasedCFG::isFallThroughSuccessor(const llvm::Instruction* stmt, const llvm::Instruction* succ) {
	return false;
}

bool LLVMBasedCFG::isBranchTarget(const llvm::Instruction* stmt, const llvm::Instruction* succ) {
	return false;
}

