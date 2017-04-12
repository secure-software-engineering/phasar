/*
 * ValueAnnotationPass.hh
 *
 *  Created on: 26.01.2017
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_VALUEANNOTATIONPASS_HH_
#define ANALYSIS_VALUEANNOTATIONPASS_HH_

#include <vector>
#include <string>
#include <iostream>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Pass.h>
#include <llvm/PassSupport.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../../utils/utils.hh"


class ValueAnnotationPass : public llvm::ModulePass {
private:
	static size_t unique_value_id;
	llvm::LLVMContext& context;
public:
	static char ID;
	ValueAnnotationPass(llvm::LLVMContext& context) : llvm::ModulePass(ID), context(context) { }
	bool runOnModule(llvm::Module& M) override;
	bool doInitialization(llvm::Module& M) override;
	bool doFinalization(llvm::Module& M) override;
	void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
	void releaseMemory() override;
};

#endif /* ANALYSIS_VALUEANNOTATIONPASS_HH_ */
