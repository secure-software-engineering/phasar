/*
 * LLVMMonotoneSolver.hh
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_MONOTONE_SOLVER_LLVMMONOTONESOLVER_HH_
#define SRC_ANALYSIS_MONOTONE_SOLVER_LLVMMONOTONESOLVER_HH_

#include "MonotoneSolver.hh"
#include "../MonotoneProblem.hh"
#include <llvm/IR/Instruction.h>
#include <iostream>
using namespace std;

template <typename D, typename C>
class LLVMMonotoneSolver : public MonotoneSolver<const llvm::Instruction*, D, const llvm::Function*, C> {
protected:
	bool DUMP_RESULTS;

public:
	LLVMMonotoneSolver();
	virtual ~LLVMMonotoneSolver() = default;

	LLVMMonotoneSolver(MonotoneProblem<const llvm::Instruction*,D,const llvm::Function*,C>& problem, bool dumpResults=false)
						: MonotoneSolver<const llvm::Instruction*,D,const llvm::Function*,C>(problem),
						  DUMP_RESULTS(dumpResults) {}

	virtual void solve() override {
		// do the solving of the analaysis problem
		MonotoneSolver<const llvm::Instruction*, D, const llvm::Function*, C>::solve();
		if (DUMP_RESULTS)
			dumpResults();
	}

	void dumpResults() {
		cout << "dump monotone solver results!\n";
	}
};

#endif /* SRC_ANALYSIS_MONOTONE_SOLVER_LLVMMONOTONESOLVER_HH_ */
