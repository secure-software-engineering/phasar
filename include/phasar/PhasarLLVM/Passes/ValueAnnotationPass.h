/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ValueAnnotationPass.h
 *
 *  Created on: 26.01.2017
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_PASSES_VALUEANNOTATIONPASS_H_
#define PHASAR_PHASARLLVM_PASSES_VALUEANNOTATIONPASS_H_

#include "llvm/IR/PassManager.h"

namespace llvm {
class LLVMContext;
class Module;
class AnalysisUsage;
} // namespace llvm

namespace psr {

/**
 * This class uses the Module Pass Mechanism of LLVM to annotate every
 * every Instruction and Global Variable of a Module with a unique ID.
 *
 * This pass obviously modifies the analyzed Module, but preserves the
 * CFG.
 *
 * @brief Annotates every Instruction with a unique ID.
 */
class ValueAnnotationPass
    : public llvm::AnalysisInfoMixin<ValueAnnotationPass> {
private:
  friend llvm::AnalysisInfoMixin<ValueAnnotationPass>;
  static llvm::AnalysisKey Key;
  static size_t unique_value_id;

public:
  explicit ValueAnnotationPass();

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

  /**
   * @brief Resets the global ID - only used for unit testing!
   */
  static void resetValueID();
};

} // namespace psr

#endif
