/*
 * MyHelloPass.hh
 *
 *  Created on: 05.07.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_GENERALSTATISTICSPASS_HH_
#define ANALYSIS_GENERALSTATISTICSPASS_HH_

#include <vector>
#include <string>
#include <set>
#include <iostream>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Pass.h>
#include <llvm/PassSupport.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/CallSite.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../../utils/utils.hh"
#include "../../utils/Logger.hh"


class GeneralStatisticsPass : public llvm::ModulePass {
private:
	size_t functions = 0;
	size_t globals = 0;
	size_t basicblocks = 0;
	size_t allocationsites = 0;
	size_t callsites = 0;
	size_t instructions = 0;
	size_t pointers = 0;
public:
	static char ID;
	GeneralStatisticsPass() : llvm::ModulePass(ID) { }
	bool runOnModule(llvm::Module& M) override;
	bool doInitialization(llvm::Module& M) override;
	bool doFinalization(llvm::Module& M) override;
	void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
	void releaseMemory() override;
	size_t getAllocationsites();
	size_t getFunctioncalls();
	size_t getInstructions();
	size_t getPointers();

};


#endif /* ANALYSIS_GENERALSTATISTICSPASS_HH_ */
