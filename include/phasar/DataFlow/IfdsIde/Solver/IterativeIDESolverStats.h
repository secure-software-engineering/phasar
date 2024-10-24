#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_ITERATIVEIDESOLVERSTATS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_ITERATIVEIDESOLVERSTATS_H

/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/FlowEdgeFunctionCacheStats.h"

#include <type_traits>

namespace psr {
struct IterativeIDESolverStats {
  FlowEdgeFunctionCacheStats FEStats;

  size_t NumAllInterPropagations = 0;
  size_t AllInterPropagationsBytes = 0;
  size_t SourceFactAndCSToInterJobSize = 0;
  size_t SourceFactAndCSToInterJobBytes = 0;
  size_t SourceFactAndFuncToInterJobSize = 0;
  size_t SourceFactAndFuncToInterJobBytes = 0;
  size_t NumPathEdges = 0;
  size_t NumPathEdgesHighWatermark = 0;
  size_t JumpFunctionsMapBytes = 0;
  size_t MaxESGEdgesPerInst = 0;
  double AvgESGEdgesPerInst = 0;
  size_t CumulESGEdges = 0;
  size_t ValTabBytes = 0;
  size_t WorkListHighWatermark = 0;
  size_t CallWLHighWatermark = 0;
  size_t WLPropHighWatermark = 0;
  size_t WLCompHighWatermark = 0;
  size_t NumFlowFacts = 0;
  size_t InstCompressorCapacity = 0;
  size_t FactCompressorCapacity = 0;
  size_t FunCompressorCapacity = 0;

  size_t TotalNumRelevantCalls = 0;
  size_t CumulNumInterJobsPerRelevantCall = 0;
  size_t MaxNumInterJobsPerRelevantCall = 0;
  size_t TotalNumLinearSearchForSummary = 0;
  size_t CumulLinearSearchLenForSummary = 0;
  size_t MaxLenLinearSearchForSummary = 0;
  size_t CumulDiffNumSummariesFound = 0;
  size_t MaxDiffNumSummariesFound = 0;
  double CumulRelDiffNumSummariesFound = 0;

  size_t NumEndSummaries = 0;
  size_t EndSummaryTabSize = 0;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const IterativeIDESolverStats &S);
};

template <typename StaticSolverConfigTy, typename = void>
struct SolverStatsSelector {};

template <typename StaticSolverConfigTy>
struct SolverStatsSelector<
    StaticSolverConfigTy,
    std::enable_if_t<StaticSolverConfigTy::EnableStatistics>>
    : IterativeIDESolverStats {};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_ITERATIVEIDESOLVERSTATS_H
