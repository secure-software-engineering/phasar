/*
 * MonotoneSolverTest.cpp
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#include "MonotoneSolverTest.hh"

MonotoneSolverTest::MonotoneSolverTest(LLVMBasedCFG& cfg, const llvm::Function* f) : MonotoneProblem<const llvm::Instruction*,
																																																		 const llvm::Value*,
																																																		 const llvm::Function*,
																																																		 LLVMBasedCFG&>(cfg, f) {}

set<const llvm::Value*> MonotoneSolverTest::join(const set<const llvm::Value*>& lhs, const set<const llvm::Value*>& rhs) {
	cout << "MonotoneSolverTest::join()\n";
	return {};
}

bool MonotoneSolverTest::sqSubSetEq(const set<const llvm::Value*>& lhs, const set<const llvm::Value*>& rhs) {
	cout << "MonotoneSolverTest::sqSubSetEq()\n";
	return false;
}

set<const llvm::Value*> MonotoneSolverTest::flow(const llvm::Instruction* s, const set<const llvm::Value*>& in) {
	cout << "MonotoneSolverTest::flow()\n";
	return {};
}

map<const llvm::Instruction*, set<const llvm::Value*>> MonotoneSolverTest::initialSeeds() {
	cout << "MonotoneSolverTest::initialSeeds()\n";
	return {};
}
