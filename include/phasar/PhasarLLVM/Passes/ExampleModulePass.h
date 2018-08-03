/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MyAliasAnalysis.h
 *
 *  Created on: 07.07.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_PASSES_EXAMPLEMODULEPASS_H_
#define PHASAR_PHASARLLVM_PASSES_EXAMPLEMODULEPASS_H_

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Pass.h>

namespace llvm {
class Module;
class AnalysisUsage;
} // namespace llvm

namespace psr {

// WARNING: Why is llvm::AAResultBase a parent of this pass ?
class ExampleModulePass : public llvm::ModulePass,
                          llvm::AAResultBase<llvm::BasicAAResult> {
public:
  static char ID;
  ExampleModulePass() : llvm::ModulePass(ID) {}
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool doInitialization(llvm::Module &M) override;
  bool runOnModule(llvm::Module &M) override;
  bool doFinalization(llvm::Module &M) override;
  void releaseMemory() override;
  //	void *getAdjustedAnalysisPointer(AnalysisID ID) override;
  //	AliasResult alias(const Value *V1, unsigned V1Size,
  //			  	  	  const Value *V2, unsigned V2Size);
};

} // namespace psr

#endif
