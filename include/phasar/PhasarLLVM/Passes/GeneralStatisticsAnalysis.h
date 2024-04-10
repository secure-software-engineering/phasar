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

#include "llvm/IR/PassManager.h"

#include "nlohmann/json.hpp"

#include <set>

namespace llvm {
class Type;
class Value;
class Instruction;
class AnalysisUsage;
class Module;
} // namespace llvm

namespace psr {

struct GeneralStatistics {

  size_t Functions = 0;
  size_t ExternalFunctions = 0;
  size_t FunctionDefinitions = 0;
  size_t AddressTakenFunctions = 0;
  size_t Globals = 0;
  size_t GlobalConsts = 0;
  size_t ExternalGlobals = 0;
  size_t GlobalsDefinitions = 0;
  size_t BasicBlocks = 0;
  size_t AllocationSites = 0;
  size_t CallSites = 0;
  size_t DebugIntrinsics = 0;
  size_t Instructions = 0;
  size_t StoreInstructions = 0;
  size_t LoadInstructions = 0;
  size_t MemIntrinsics = 0;
  size_t Branches = 0;
  size_t Switches = 0;
  size_t GetElementPtrs = 0;
  size_t LandingPads = 0;
  size_t PhiNodes = 0;
  size_t NumInlineAsm = 0;
  size_t IndCalls = 0;
  size_t TotalNumOperands = 0;
  size_t TotalNumUses = 0;
  size_t TotalNumPredecessorBBs = 0;
  size_t TotalNumSuccessorBBs = 0;
  size_t MaxNumOperands = 0;
  size_t MaxNumUses = 0;
  size_t MaxNumPredecessorBBs = 0;
  size_t MaxNumSuccessorBBs = 0;
  size_t NumInstWithMultipleUses = 0;
  size_t NumInstsUsedOutsideBB = 0;
  size_t NonVoidInsts = 0;
  std::set<const llvm::Type *> AllocatedTypes;
  std::set<const llvm::Instruction *> AllocaInstructions;
  std::set<const llvm::Instruction *> RetResInstructions;
  std::string ModuleName{};

  /**
   * @brief Returns the number of Allocation sites.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use AllocationSites instead")]] size_t
  getAllocationsites() const;

  /**
   * @brief Returns the number of Function calls.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use CallSites instead")]] size_t
  getFunctioncalls() const;

  /**
   * @brief Returns the number of Instructions.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use Instructions instead")]] size_t
  getInstructions() const;

  /**
   * @brief Returns the number of global pointers.
   */
  [[nodiscard]] [[deprecated(
      "All globals are pointers. Use Globals instead")]] size_t
  getGlobalPointers() const;

  /**
   * @brief Returns the number of basic blocks.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use BasicBlocks instead")]] size_t
  getBasicBlocks() const;

  /**
   * @brief Returns the number of functions.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use Functions instead")]] size_t
  getFunctions() const;

  /**
   * @brief Returns the number of globals.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use Globals instead")]] size_t
  getGlobals() const;

  /**
   * @brief Returns the number of constant globals.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use GlobalConsts instead")]] size_t
  getGlobalConsts() const;

  /**
   * @brief Returns the number of memory intrinsics.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use MemIntrinsics instead")]] size_t
  getMemoryIntrinsics() const;

  /**
   * @brief Returns the number of store instructions.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use StoreInstructions instead")]] size_t
  getStoreInstructions() const;

  /**
   * @brief Returns the number of load instructions.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use LoadInstructions instead; this "
      "function seems to be broken anyway")]] size_t
  getLoadInstructions();

  /**
   * @brief Returns all possible Types.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use AllocatedTypes instead")]] const std::
      set<const llvm::Type *> &
      getAllocatedTypes() const;

  /**
   * @brief Returns all stack and heap allocating instructions.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use AllocaInstructions "
      "instead")]] const std::set<const llvm::Instruction *> &
  getAllocaInstructions() const;

  /**
   * @brief Returns all Return and Resume Instructions.
   */
  [[nodiscard]] [[deprecated(
      "Getters are no longer needed. Use RetResInstructions "
      "instead")]] const std::set<const llvm::Instruction *> &
  getRetResInstructions() const;

  [[nodiscard]] [[deprecated(
      "Please use printAsJson() instead")]] nlohmann::json
  getAsJson() const;
  void printAsJson(llvm::raw_ostream &OS = llvm::outs()) const;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const GeneralStatistics &Statistics);

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

  GeneralStatistics runOnModule(llvm::Module &M);

  inline GeneralStatistics run(llvm::Module &M,
                               llvm::ModuleAnalysisManager & /*AM*/) {
    return runOnModule(M);
  }
};

} // namespace psr

#endif
