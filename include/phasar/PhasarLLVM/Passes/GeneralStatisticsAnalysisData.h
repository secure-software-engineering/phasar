/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PASSES_GENERALSTATISTICSANALYSISDATA_H
#define PHASAR_PHASARLLVM_PASSES_GENERALSTATISTICSANALYSISDATA_H

#include "llvm/Support/raw_ostream.h"

#include <cstddef>
#include <set>
#include <string>

namespace psr {
struct GeneralStatisticsAnalysisData {
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
  // const llvm::Type * , serialized with MetadataID
  std::set<size_t> AllocatedTypes;
  // const llvm::Instruction * , serialized with MetadataID
  std::set<size_t> AllocaInstructions;
  // const llvm::Instruction * , serialized with MetadataID
  std::set<size_t> RetResInstructions;
  std::string ModuleName{};

  GeneralStatisticsAnalysisData() noexcept = default;
  void printAsJson(llvm::raw_ostream &OS);

  static GeneralStatisticsAnalysisData deserializeJson(const llvm::Twine &Path);
  static GeneralStatisticsAnalysisData
  loadJsonString(llvm::StringRef JsonAsString);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_PASSES_GENERALSTATISTICSANALYSISDATA_H
