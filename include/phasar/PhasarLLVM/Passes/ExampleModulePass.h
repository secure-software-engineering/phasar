/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PASSES_EXAMPLEMODULEPASS_H_
#define PHASAR_PHASARLLVM_PASSES_EXAMPLEMODULEPASS_H_

#include <llvm/IR/PassManager.h>

namespace llvm {
class Module;
} // namespace llvm

namespace psr {

class ExampleModulePass : public llvm::AnalysisInfoMixin<ExampleModulePass> {
private:
  friend llvm::AnalysisInfoMixin<ExampleModulePass>;
  static llvm::AnalysisKey Key;

public:
  explicit ExampleModulePass();

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
};

} // namespace psr

#endif
