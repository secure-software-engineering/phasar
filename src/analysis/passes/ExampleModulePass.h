/*
 * MyAliasAnalysis.h
 *
 *  Created on: 07.07.2016
 *      Author: pdschbrt
 */

#ifndef EXAMPLEMODULEPASS_H_
#define EXAMPLEMODULEPASS_H_

#include <llvm/ADT/SCCIterator.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

using namespace llvm;

class ExampleModulePass : public ModulePass, AAResultBase<BasicAAResult> {
public:
  static char ID;
  ExampleModulePass() : ModulePass(ID) {}
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool doInitialization(llvm::Module &M) override;
  bool runOnModule(llvm::Module &M) override;
  bool doFinalization(llvm::Module &M) override;
  void releaseMemory() override;
  //	void *getAdjustedAnalysisPointer(AnalysisID ID) override;
  //	AliasResult alias(const Value *V1, unsigned V1Size,
  //			  	  	  const Value *V2, unsigned V2Size);
};

#endif /* ANALYSIS_MYALIASANALYSISPASS_HH_ */
