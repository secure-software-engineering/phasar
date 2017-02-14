/*
 * MyAliasAnalysis.cpp
 *
 *  Created on: 07.07.2016
 *      Author: pdschbrt
 */

#include "ExampleModulePass.hh"


void MyAliasAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	//AAResult::gettAnalysisUsage(AU);
	// my own dependencies can follow
	AU.setPreservesAll();
}

bool MyAliasAnalysisPass::doInitialization(llvm::Module &M)
{
	outs() << "MyAA: doInitialization()\n";
	return false;
}

bool MyAliasAnalysisPass::doFinalization(llvm::Module &M)
{
	outs() << "MyAA: doFinalization()\n";
	return false;
}

bool MyAliasAnalysisPass::runOnModule(llvm::Module &M)
{
	// perform analysis here ...
	outs() << "MyAA: runOnModule() with name: " << M.getName() << "\n";
	// iterate over functions
	for (Module::iterator MI = M.begin(); MI != M.end(); ++MI) {
		outs() << "found function: " << MI->getName() << "\n";
		// iterate over basic blocks
		// MI->viewCFG();
		for (Function::iterator FI = MI->begin(); FI != MI->end(); ++FI) {
			ilist_iterator<BasicBlock> BB = FI;
		    // iterate over single instructions
			for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
		    	Instruction &I = *BI++;
		        I.print(outs() << "\n", true);
		    }
		}
	}
	CallGraph CG = CallGraph(M);
	CG.dump();
	for (CallGraph::const_iterator CGI = CG.begin(); CGI != CG.end(); ++CGI) {
//		outs() << "CGI start: " << CGI->first << "CGI end\n";
	}
	return false;
}

//void MyAliasAnalysisPass::*getAdjustedAliasPointer(const void* ID)
//{
////	if (ID == &AAResultBase::ID)
////		return (MyAliasAnalysisPass*) this;
////	return (AAResults*) this;
//}
//
//AliasResult MyAliasAnalysisPass::alias(const Value *V1, unsigned V1Size,
//		  	  	  const Value *V2, unsigned V2Size)
//{
//	if (1 == 2)
//		return NoAlias;
//	// could not determine a must or no-alias result
//	return AAResultBase::alias(MemoryLocation(V1, V1Size), MemoryLocation(V2, V2Size));
//}

void MyAliasAnalysisPass::releaseMemory()
{
	outs() << "\nMyAA: memory released!\n";
}

