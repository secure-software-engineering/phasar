#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolver.h"
#include "phasar/DataFlow/IfdsIde/Solver/StaticIDESolverConfig.h"
#include "phasar/DataFlow/PathSensitivity/ExplodedSuperGraph.h"

namespace psr {
template <typename Base> struct PathAwareIDESolverConfig {
  template <typename AnalysisDomainTy>
  using PathTrackingData = ExplodedSuperGraph<AnalysisDomainTy>;

  template <typename ProblemTy>
  static PathTrackingData<typename ProblemTy::ProblemAnalysisDomain>
  initPathData(ProblemTy &Problem) {
    return ExplodedSuperGraph<typename ProblemTy::ProblemAnalysisDomain>(
        Problem.getZeroValue());
  }

  template <typename AnalysisDomainTy>
  static void saveEdges(PathTrackingData<AnalysisDomainTy> &Data,
                        ByConstRef<typename AnalysisDomainTy::n_t> Curr,
                        ByConstRef<typename AnalysisDomainTy::n_t> Succ,
                        ByConstRef<typename AnalysisDomainTy::d_t> CurrNode,
                        const auto &SuccNodes, ESGEdgeKind Kind) {
    Data.saveEdges(Curr, CurrNode, Succ, SuccNodes, Kind);
  }
};

template <typename ProblemTy,
          typename StaticSolverConfigTy = DefaultIDESolverConfig<ProblemTy>,
          ICFG ICFGTy = typename ProblemTy::ProblemAnalysisDomain::i_t>
class PathAwareIterIDESolver
    : public IterativeIDESolver<
          ProblemTy, PathAwareIDESolverConfig<StaticSolverConfigTy>, ICFGTy> {
public:
  using IterativeIDESolver<ProblemTy,
                           PathAwareIDESolverConfig<StaticSolverConfigTy>,
                           ICFGTy>::IterativeIDESolver;
};

} // namespace psr
