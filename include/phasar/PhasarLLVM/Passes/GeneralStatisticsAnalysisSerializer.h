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
struct GeneralStatisticsAnalysisSerializer {
  size_t Functions = 0;
  size_t ExternalFunctions = 0;
  size_t FunctionDefinitions = 0;
  size_t AddressTakenFunctions = 0;
  size_t Globals = 0;
  size_t GlobalConsts = 0;
  size_t BasicBlocks = 0;
  size_t CallSites = 0;
  size_t DebugIntrinsics = 0;
  size_t Instructions = 0;
  size_t MemIntrinsics = 0;
  size_t Branches = 0;
  size_t GetElementPtrs = 0;
  size_t LandingPads = 0;
  size_t PhiNodes = 0;
  size_t NumInlineAsm = 0;
  size_t IndCalls = 0;
  size_t NonVoidInsts = 0;
  size_t NumberOfAllocaInstructions = 0;
  std::string ModuleName{};

  GeneralStatisticsAnalysisSerializer() noexcept = default;
  void printAsJson(llvm::raw_ostream &OS);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_PASSES_GENERALSTATISTICSANALYSISDATA_H
