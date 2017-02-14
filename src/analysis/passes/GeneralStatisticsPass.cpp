/*
 * MyHelloPass.cpp
 *
 *  Created on: 05.07.2016
 *      Author: pdschbrt
 */

#include "GeneralStatisticsPass.hh"


bool  GeneralStatisticsPass::runOnModule(llvm::Module& M)
{
	std::cout << "Running GeneralStatisticsPass ..." << std::endl;
	const std::vector<std::string> dyn_alloc_functions = { "malloc", "calloc", "realloc", "new" };
	for (llvm::Module::iterator MI = M.begin(); MI != M.end(); ++MI) {
		for (llvm::Function::iterator FI = MI->begin(); FI != MI->end(); ++FI) {
			llvm::ilist_iterator<llvm::BasicBlock> BB = FI;
		    for (llvm::BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
		    	llvm::Instruction &I = *BI++;
		    	// found one more instruction
		    	++instructions;
		    	// check for function calls
		    	if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) ++callsites;
		    	// check for calls to malloc, calloc, realloc and new
		    	if (llvm::isa<llvm::CallInst>(I)) {
		    		llvm::CallInst* call = llvm::dyn_cast<llvm::CallInst>(&I);
		    		if (call->getCalledFunction()) {
		    			for (auto dyn_alloc : dyn_alloc_functions) {
		    				if (call->getCalledFunction()->getName().str().find(dyn_alloc) != std::string::npos) {
		    					++allocationsites;
		    				}
		    			}
		    		}
		    	}
		    	if (llvm::isa<llvm::InvokeInst>(I)) {
		    		llvm::InvokeInst* invoke = llvm::dyn_cast<llvm::InvokeInst>(&I);
		    		if (invoke->getCalledFunction()) {
		    			for (auto dyn_alloc : dyn_alloc_functions) {
		    				if (invoke->getCalledFunction()->getName().str().find(dyn_alloc) != std::string::npos) {
		    					++allocationsites;
		    				}
		    			}
		    		}
		    	}
		    	// check for alloc instructions, that allocate a local pointer type
		    	if (llvm::isa<llvm::AllocaInst>(I)) {
		    		llvm::AllocaInst* alloc = llvm::dyn_cast<llvm::AllocaInst>(&I);
		    		if (alloc->getType()->isPointerTy()) {
		    			++pointers;
		    		}
		    	}
		    }
		}
	}
	// check for global pointers
	for (auto& global : M.globals()) {
		if (global.getType()->isPointerTy()) {
			++pointers;
		}
	}
	std::cout << "Done running GeneralStatisticsPass." << std::endl;
	return false;
}

bool GeneralStatisticsPass::doInitialization(llvm::Module& M) {	return false; }

bool GeneralStatisticsPass::doFinalization(llvm::Module& M)
{
	llvm::outs() << "GeneralStatisticsPass for module " << M.getName().str() << "\n";
	llvm::outs() << "allocation sites: " << allocationsites << "\n";
	llvm::outs() << "calls-sites: " << callsites << "\n";
	llvm::outs() << "pointer variables: " << pointers << "\n";
	llvm::outs() << "instructions: " << instructions << "\n";
	return false;
}

void GeneralStatisticsPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const { AU.setPreservesAll(); }

void GeneralStatisticsPass::releaseMemory() {}

size_t GeneralStatisticsPass::getAllocationsites() { return allocationsites; }

size_t GeneralStatisticsPass::getFunctioncalls() { return callsites; }

size_t GeneralStatisticsPass::getInstructions() { return instructions; }

size_t GeneralStatisticsPass::getPointers() { return pointers; }

