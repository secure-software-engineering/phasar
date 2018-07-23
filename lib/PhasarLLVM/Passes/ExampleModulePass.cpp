/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MyAliasAnalysis.cpp
 *
 *  Created on: 07.07.2016
 *      Author: pdschbrt
 */

#include <llvm/ADT/SCCIterator.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <phasar/PhasarLLVM/Passes/ExampleModulePass.h>

using namespace std;
using namespace psr;
using namespace llvm;

namespace psr {

void ExampleModulePass::getAnalysisUsage(AnalysisUsage &AU) const {
  // AAResult::gettAnalysisUsage(AU);
  // my own dependencies can follow
  AU.setPreservesAll();
}

bool ExampleModulePass::doInitialization(llvm::Module &M) {
  outs() << "MyAA: doInitialization()\n";
  return false;
}

bool ExampleModulePass::doFinalization(llvm::Module &M) {
  outs() << "MyAA: doFinalization()\n";
  return false;
}

bool ExampleModulePass::runOnModule(llvm::Module &M) {
  // perform analysis here ...
  outs() << "MyAA: runOnModule() with name: " << M.getName() << "\n";
  // iterate over functions
  for (auto &F : M) {
    outs() << "found function: " << F.getName() << "\n";
    // iterate over basic blocks
    // MI->viewCFG();
    for (auto &BB : F) {
      // iterate over single instructions
      for (auto &I : BB) {
        I.print(outs() << "\n", true);
      }
    }
  }
  CallGraph CG = CallGraph(M);
  CG.print(llvm::outs());
  for (CallGraph::const_iterator CGI = CG.begin(); CGI != CG.end(); ++CGI) {
    //		outs() << "CGI start: " << CGI->first << "CGI end\n";
  }
  return false;
}

// void MyAliasAnalysisPass::*getAdjustedAliasPointer(const void* ID)
//{
////	if (ID == &AAResultBase::ID)
////		return (MyAliasAnalysisPass*) this;
////	return (AAResults*) this;
//}
//
// AliasResult MyAliasAnalysisPass::alias(const Value *V1, unsigned V1Size,
//		  	  	  const Value *V2, unsigned V2Size)
//{
//	if (1 == 2)
//		return NoAlias;
//	// could not determine a must or no-alias result
//	return AAResultBase::alias(MemoryLocation(V1, V1Size),
// MemoryLocation(V2, V2Size));
//}

void ExampleModulePass::releaseMemory() {
  outs() << "\nMyAA: memory released!\n";
}

} // namespace psr
