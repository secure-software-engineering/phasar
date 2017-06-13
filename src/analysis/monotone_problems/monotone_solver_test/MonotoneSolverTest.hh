/*
 * MonotoneSolverTest.hh
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef MONOTONESOLVERTEST_HH_
#define MONOTONESOLVERTEST_HH_

#include <llvm/IR/Value.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <iostream>
#include "../../icfg/LLVMBasedCFG.hh"
#include "../../monotone/MonotoneProblem.hh"
using namespace std;

class MonotoneSolverTest : public MonotoneProblem<const llvm::Instruction*,
																									const llvm::Value*,
																									const llvm::Function*,
																									LLVMBasedCFG&> {
public:
	MonotoneSolverTest(LLVMBasedCFG& cfg, const llvm::Function* f);
	virtual ~MonotoneSolverTest() = default;
	virtual set<const llvm::Value*> join(const set<const llvm::Value*>& lhs, const set<const llvm::Value*>& rhs) override;
	virtual bool sqSubSetEq(const set<const llvm::Value*>& lhs, const set<const llvm::Value*>& rhs) override;
	virtual set<const llvm::Value*> flow(const llvm::Instruction* s, const set<const llvm::Value*>& in) override;
	virtual map<const llvm::Instruction*, set<const llvm::Value*>> initialSeeds() override;
};

#endif
