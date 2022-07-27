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

#include "llvm/IR/PassManager.h"

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
  size_t Functions = 0;
  size_t Globals = 0;
  size_t BasicBlocks = 0;
  size_t AllocationSites = 0;
  size_t CallSites = 0;
  size_t Instructions = 0;
  size_t StoreInstructions = 0;
  size_t LoadInstructions = 0;
  size_t MemIntrinsics = 0;
  size_t GlobalPointers = 0;
  std::set<const llvm::Type *> AllocatedTypes;
  std::set<const llvm::Instruction *> AllocaInstructions;
  std::set<const llvm::Instruction *> RetResInstructions;

public:
  /**
   * @brief Returns the number of Allocation sites.
   */
  [[nodiscard]] size_t getAllocationsites() const;

  /**
   * @brief Returns the number of Function calls.
   */
  [[nodiscard]] size_t getFunctioncalls() const;

  /**
   * @brief Returns the number of Instructions.
   */
  [[nodiscard]] size_t getInstructions() const;

  /**
   * @brief Returns the number of global pointers.
   */
  [[nodiscard]] size_t getGlobalPointers() const;

  /**
   * @brief Returns the number of basic blocks.
   */
  [[nodiscard]] size_t getBasicBlocks() const;

  /**
   * @brief Returns the number of functions.
   */
  [[nodiscard]] size_t getFunctions() const;

  /**
   * @brief Returns the number of globals.
   */
  [[nodiscard]] size_t getGlobals() const;

  /**
   * @brief Returns the number of memory intrinsics.
   */
  [[nodiscard]] size_t getMemoryIntrinsics() const;

  /**
   * @brief Returns the number of store instructions.
   */
  [[nodiscard]] size_t getStoreInstructions() const;

  /**
   * @brief Returns the number of load instructions.
   */
  [[nodiscard]] size_t getLoadInstructions();

  /**
   * @brief Returns all possible Types.
   */
  [[nodiscard]] std::set<const llvm::Type *> getAllocatedTypes() const;

  /**
   * @brief Returns all stack and heap allocating instructions.
   */
  [[nodiscard]] std::set<const llvm::Instruction *>
  getAllocaInstructions() const;

  /**
   * @brief Returns all Return and Resume Instructions.
   */
  [[nodiscard]] std::set<const llvm::Instruction *>
  getRetResInstructions() const;
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

  explicit GeneralStatisticsAnalysis() = default;

  GeneralStatistics run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

} // namespace psr

#endif
