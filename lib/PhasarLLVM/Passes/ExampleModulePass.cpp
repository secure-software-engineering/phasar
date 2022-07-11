/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "llvm/IR/Module.h"

#include "phasar/PhasarLLVM/Passes/ExampleModulePass.h"

using namespace std;
using namespace psr;

namespace psr {

llvm::AnalysisKey ExampleModulePass::Key;

ExampleModulePass::ExampleModulePass() = default;

llvm::PreservedAnalyses
ExampleModulePass::run(llvm::Module & /*M*/,
                       llvm::ModuleAnalysisManager & /*MAM*/) {
  llvm::outs() << "ExampleModulePass::run()\n";
  return llvm::PreservedAnalyses::all();
}

} // namespace psr
