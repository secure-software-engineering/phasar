/*
 * MonotoneSolverTest.cpp
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#include "MonotoneSolverTest.hh"

MonotoneSolverTest::MonotoneSolverTest(LLVMBasedCFG& cfg) {

}

set<const llvm::Value*> MonotoneSolverTest::join(const set<const llvm::Value*>& lhs, const set<const llvm::Value*>& rhs) {
	return {};
}

bool MonotoneSolverTest::sqSubSetEq(const set<const llvm::Value*>& lhs, const set<const llvm::Value*>& rhs) {
	return false;
}

set<const llvm::Value*> MonotoneSolverTest::flow(const llvm::Instruction* s, const set<const llvm::Value*>& in) {
	return {};
}

map<const llvm::Instruction*, set<const llvm::Value*>> MonotoneSolverTest::initialSeeds() {
	return {};
}
