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

#ifndef PHASAR_PHASARLLVM_PASSES_GENERALSTATISTICSANALYSIS_H_
#define PHASAR_PHASARLLVM_PASSES_GENERALSTATISTICSANALYSIS_H_

#include <set>

#include <llvm/IR/PassManager.h>

namespace llvm {
class Type;
class Value;
class Instruction;
class AnalysisUsage;
class Module;
} // namespace llvm

namespace psr {

class GeneralStatistics {
private:
  friend class GeneralStatisticsAnalysis;
  size_t functions = 0;
  size_t globals = 0;
  size_t basicblocks = 0;
  size_t allocationsites = 0;
  size_t callsites = 0;
  size_t instructions = 0;
  size_t storeInstructions = 0;
  size_t loadInstructions = 0;
  size_t memIntrinsic = 0;
  size_t globalPointers = 0;
  std::set<const llvm::Type *> allocatedTypes;
  std::set<const llvm::Instruction *> allocaInstructions;
  std::set<const llvm::Instruction *> retResInstructions;

public:
  /**
   * @brief Returns the number of Allocation sites.
   */
  size_t getAllocationsites() const;

  /**
   * @brief Returns the number of Function calls.
   */
  size_t getFunctioncalls() const;

  /**
   * @brief Returns the number of Instructions.
   */
  size_t getInstructions() const;

  /**
   * @brief Returns the number of global pointers.
   */
  size_t getGlobalPointers() const;

  /**
   * @brief Returns the number of basic blocks.
   */
  size_t getBasicBlocks() const;

  /**
   * @brief Returns the number of functions.
   */
  size_t getFunctions() const;

  /**
   * @brief Returns the number of globals.
   */
  size_t getGlobals() const;

  /**
   * @brief Returns the number of memory intrinsics.
   */
  size_t getMemoryIntrinsics() const;

  /**
   * @brief Returns the number of store instructions.
   */
  size_t getStoreInstructions() const;

  /**
   * @brief Returns the number of load instructions.
   */
  size_t getLoadInstructions();

  /**
   * @brief Returns all possible Types.
   */
  std::set<const llvm::Type *> getAllocatedTypes() const;

  /**
   * @brief Returns all stack and heap allocating instructions.
   */
  std::set<const llvm::Instruction *> getAllocaInstructions() const;

  /**
   * @brief Returns all Return and Resume Instructions.
   */
  std::set<const llvm::Instruction *> getRetResInstructions() const;
};

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
class GeneralStatisticsAnalysis
    : public llvm::AnalysisInfoMixin<GeneralStatisticsAnalysis> {
private:
  friend llvm::AnalysisInfoMixin<GeneralStatisticsAnalysis>;
  static llvm::AnalysisKey Key;
  GeneralStatistics Stats;

public:
  /// The pass itself stores the results.
  using Result = GeneralStatistics;

  explicit GeneralStatisticsAnalysis();

  GeneralStatistics run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

} // namespace psr

#endif
