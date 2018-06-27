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

#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/PassSupport.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_os_ostream.h>
#include <phasar/Config/Configuration.h>
#include <phasar/Utils/Logger.h>


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
class ValueAnnotationPass : public llvm::ModulePass {
private:
  static size_t unique_value_id;
  llvm::LLVMContext &context;

public:
  // TODO What's the ID good for?
  static char ID;
  // TODO What exactly does the constructor do?
  ValueAnnotationPass(llvm::LLVMContext &context)
      : llvm::ModulePass(ID), context(context) {}

  /**
   * @brief Does the annotation.
   * @param M The analyzed Module.
   * @return Always true.
   */
  bool runOnModule(llvm::Module &M) override;

  /**
   * @brief Not used in this context!
   * @return Always false.
   */
  bool doInitialization(llvm::Module &M) override;

  /**
   * @brief Not used in this context!
   * @return Always false.
   */
  bool doFinalization(llvm::Module &M) override;

  /**
   * @brief Sets that the pass preserves the CFG.
   */
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  /**
   * This pass holds onto memory for the entire duration of their lifetime
   * (which is the entire compile time). This is the default behavior for
   * passes.
   *
   * @brief The pass does not release any memory during their lifetime.
   */
  void releaseMemory() override;

  /**
   * @brief Resets the global ID - only used for unit testing!
   */
  static void resetValueID();
};

} // namespace psr
