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

#include <set>
#include <vector>

namespace llvm {
class Type;
class Value;
class Instruction;
class AnalysisUsage;
class Module;
class DICompositeType;
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
  std::vector<const llvm::DICompositeType *> AllocatedTypes;
  std::set<const llvm::Instruction *> AllocaInstructions;
  std::set<const llvm::Instruction *> RetResInstructions;
  std::string ModuleName{};

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
