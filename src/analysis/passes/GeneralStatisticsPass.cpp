/*
 * MyHelloPass.cpp
 *
 *  Created on: 05.07.2016
 *      Author: pdschbrt
 */

#include "GeneralStatisticsPass.hh"


bool  GeneralStatisticsPass::runOnModule(llvm::Module& M)
{
	auto& lg = lg::get();
	BOOST_LOG_SEV(lg, INFO) << "Running GeneralStatisticsPass";
	const std::set<std::string> mem_allocating_functions = { "operator new(unsigned long)",
																												   "operator new[](unsigned long)",
																													 "malloc",
																													 "calloc",
																													 "realloc" };
	for (auto& F : M) {
		++functions;
		for (auto& BB : F) {
				++basicblocks;
		    for (auto& I : BB) {
		    	// found one more instruction
		    	++instructions;
		    	// check for function calls
		    	if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
		    		++callsites;
		    		llvm::ImmutableCallSite CS(&I);
		    		if (CS.getCalledFunction()) {
		    			if (mem_allocating_functions.find(cxx_demangle(CS.getCalledFunction()->getName().str())) != mem_allocating_functions.end()) {
		    				++allocationsites;
		    			}
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
		++globals;
	}
	return false;
}

bool GeneralStatisticsPass::doInitialization(llvm::Module& M) {	return false; }

bool GeneralStatisticsPass::doFinalization(llvm::Module& M)
{
	llvm::outs() << "GeneralStatisticsPass summary for module: '" << M.getName().str() << "'\n";
	llvm::outs() << "functions: " << functions << "\n";
	llvm::outs() << "globals: " << globals << "\n";
	llvm::outs() << "basic blocks: " << basicblocks << "\n";
	llvm::outs() << "allocation sites: " << allocationsites << "\n";
	llvm::outs() << "calls-sites: " << callsites << "\n";
	llvm::outs() << "pointer variables: " << pointers << "\n";
	llvm::outs() << "instructions: " << instructions << "\n\n";
	return false;
}

void GeneralStatisticsPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const { AU.setPreservesAll(); }

void GeneralStatisticsPass::releaseMemory() {}

size_t GeneralStatisticsPass::getAllocationsites() { return allocationsites; }

size_t GeneralStatisticsPass::getFunctioncalls() { return callsites; }

size_t GeneralStatisticsPass::getInstructions() { return instructions; }

size_t GeneralStatisticsPass::getPointers() { return pointers; }

