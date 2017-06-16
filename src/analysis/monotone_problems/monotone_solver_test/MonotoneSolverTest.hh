/*
 * MonotoneSolverTest.hh
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef MONOTONESOLVERTEST_HH_
#define MONOTONESOLVERTEST_HH_

#include <algorithm>
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
	MonotoneSolverTest(LLVMBasedCFG& Cfg, const llvm::Function* F);
	virtual ~MonotoneSolverTest() = default;
	virtual MonoSet<const llvm::Value*> join(const MonoSet<const llvm::Value*>& Lhs, const MonoSet<const llvm::Value*>& Rhs) override;
	virtual bool sqSubSetEqual(const MonoSet<const llvm::Value*>& Lhs, const MonoSet<const llvm::Value*>& Rhs) override;
	virtual MonoSet<const llvm::Value*> flow(const llvm::Instruction* S, const MonoSet<const llvm::Value*>& In) override;
	virtual MonoMap<const llvm::Instruction*, MonoSet<const llvm::Value*>> initialSeeds() override;
};

#endif
