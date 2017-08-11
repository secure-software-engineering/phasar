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

/**
 * This class uses the Module Pass Mechanism of LLVM to compute
 * some statistics about a Module. This includes the number of
 *  - Functions
 *  - Global variables
 *  - Basic blocks
 *  - Allocation sites
 *  - Call sites
 *  - Instructions
 *  - Pointers
 *
 *  and also a set of all allocated Types in that Module.
 *
 * @brief Computes general statistics for a llvm::Module.
 */
class GeneralStatisticsPass : public llvm::ModulePass {
private:
	size_t functions = 0;
	size_t globals = 0;
	size_t basicblocks = 0;
	size_t allocationsites = 0;
	size_t callsites = 0;
	size_t instructions = 0;
	size_t pointers = 0;
	set<const llvm::Type*> allocatedTypes;
public:
  // TODO What's the ID good for?
	static char ID;
	GeneralStatisticsPass() : llvm::ModulePass(ID) { }

  /**
   * @brief Does all the computation of the statistics.
   * @param M The analyzed Module.
   * @return Always false.
   */
  bool runOnModule(llvm::Module& M) override;

  /**
   * @brief Not used in this context!
   * @return Always false.
   */
  bool doInitialization(llvm::Module& M) override;

  /**
   * @brief Prints the computed statistics to the command-line
   * @param M The analyzed Module.
   * @return Always false;
   */
	bool doFinalization(llvm::Module& M) override;
	void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
	void releaseMemory() override;
	size_t getAllocationsites();
	size_t getFunctioncalls();
	size_t getInstructions();
	size_t getPointers();
  set<const llvm::Type*> getAllocatedTypes();
};


#endif /* ANALYSIS_GENERALSTATISTICSPASS_HH_ */
