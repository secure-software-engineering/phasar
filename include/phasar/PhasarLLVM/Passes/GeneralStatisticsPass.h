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

#ifndef ANALYSIS_GENERALSTATISTICSPASS_H_
#define ANALYSIS_GENERALSTATISTICSPASS_H_

#include <iostream>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include "llvm/IR/IntrinsicInst.h"
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/PassSupport.h>
#include <llvm/Support/raw_os_ostream.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>
#include <phasar/Utils/PAMM.h>
#include <set>
#include <string>
#include <vector>

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
  size_t pointers = 0;
  set<const llvm::Type *> allocatedTypes;
  set<const llvm::Value *> allocaInstrucitons;
  set<const llvm::Instruction *> retResInstructions;

public:
  // TODO What's the ID good for?
  static char ID;
  // TODO What exactly does the constructor do?
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
   * @brief Returns the number of Pointers.
   */
  size_t getPointers();

  /**
   * @brief Returns all possible Types.
   */
  set<const llvm::Type *> getAllocatedTypes();

  /**
 * @brief Returns all stack and heap allocating instructions.
 */
  set<const llvm::Value *> getAllocaInstructions();

  /**
   * @brief Returns all Return and Resume Instructions.
   */
  set<const llvm::Instruction *> getRetResInstructions();
};

#endif /* ANALYSIS_GENERALSTATISTICSPASS_HH_ */
