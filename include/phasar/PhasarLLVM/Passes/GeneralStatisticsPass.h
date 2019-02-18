/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MyHelloPass.h
 *
 *  Created on: 05.07.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_PASSES_GENERALSTATISTICSPASS_H_
#define PHASAR_PHASARLLVM_PASSES_GENERALSTATISTICSPASS_H_

#include <set>

#include <llvm/Pass.h>

namespace llvm {
class Type;
class Value;
class Instruction;
class AnalysisUsage;
class Module;
} // namespace llvm

namespace psr {

/**
 * This class uses the Module Pass Mechanism of LLVM to compute
 * some statistics about a Module. This includes the number of
 *  - Function calls
 *  - Global variables
 *  - Basic blocks
 *  - Allocation sites
 *  - Call sites
 *  - Instructions
 *  - Pointers
 *
 *  and also a set of all allocated Types in that Module.
 *
 *  This pass does not modify the analyzed Module in any way!
 *
 * @brief Computes general statistics for a Module.
 */
class GeneralStatisticsPass : public llvm::ModulePass {
private:
  size_t functions = 0;
  size_t globals = 0;
  size_t basicblocks = 0;
  size_t allocationsites = 0;
  size_t callsites = 0;
  size_t instructions = 0;
  size_t storeInstructions = 0;
  size_t memIntrinsic = 0;
  size_t globalPointers = 0;
  std::set<const llvm::Type *> allocatedTypes;
  std::set<const llvm::Value *> allocaInstrucitons;
  std::set<const llvm::Instruction *> retResInstructions;

public:
  static char ID;
  GeneralStatisticsPass() : llvm::ModulePass(ID) {}

  /**
   * @brief Does all the computation of the statistics.
   * @param M The analyzed Module.
   * @return Always false.
   */
  bool runOnModule(llvm::Module &M) override;

  /**
   * @brief Not used in this context!
   * @return Always false.
   */
  bool doInitialization(llvm::Module &M) override;

  /**
   * @brief Prints the computed statistics to the command-line
   * @param M The analyzed Module.
   * @return Always false;
   */
  bool doFinalization(llvm::Module &M) override;

  /**
   * @brief Sets that the pass does not transform its input at all.
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
   * @brief Returns the number of Allocation sites.
   */
  size_t getAllocationsites();

  /**
   * @brief Returns the number of Function calls.
   */
  size_t getFunctioncalls();

  /**
   * @brief Returns the number of Instructions.
   */
  size_t getInstructions();

  /**
   * @brief Returns the number of global pointers.
   */
  size_t getGlobalPointers();

  /**
   * @brief Returns all possible Types.
   */
  std::set<const llvm::Type *> getAllocatedTypes();

  /**
   * @brief Returns all stack and heap allocating instructions.
   */
  std::set<const llvm::Value *> getAllocaInstructions();

  /**
   * @brief Returns all Return and Resume Instructions.
   */
  std::set<const llvm::Instruction *> getRetResInstructions();
};

} // namespace psr

#endif
