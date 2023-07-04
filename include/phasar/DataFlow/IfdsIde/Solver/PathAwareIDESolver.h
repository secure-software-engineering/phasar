/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_PATHAWAREIDESOLVER_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_PATHAWAREIDESOLVER_H

#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/Solver/ESGEdgeKind.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/DataFlow/PathSensitivity/ExplodedSuperGraph.h"
#include "phasar/Utils/Logger.h"

namespace psr {
template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class PathAwareIDESolver : public IDESolver<AnalysisDomainTy, Container> {
  using base_t = IDESolver<AnalysisDomainTy, Container>;

public:
  using domain_t = AnalysisDomainTy;
  using n_t = typename base_t::n_t;
  using d_t = typename base_t::d_t;
  using i_t = typename base_t::i_t;
  using container_type = typename base_t::container_type;

  explicit PathAwareIDESolver(
      IDETabulationProblem<domain_t, container_type> &Problem, const i_t *ICF)
      : base_t(Problem, ICF), ESG(Problem.getZeroValue(), Problem, Problem) {

    if (Problem.getIFDSIDESolverConfig().autoAddZero()) {
      PHASAR_LOG_LEVEL(
          WARNING,
          "The PathAwareIDESolver is initialized with the option 'autoAddZero' "
          "being set. This might degrade the quality of the computed paths!");
    }
  }

  [[nodiscard]] const ExplodedSuperGraph<domain_t> &
  getExplicitESG() const &noexcept {
    return ESG;
  }

  [[nodiscard]] ExplodedSuperGraph<domain_t> &&getExplicitESG() &&noexcept {
    return std::move(ESG);
  }

private:
  void saveEdges(n_t Curr, n_t Succ, d_t CurrNode,
                 const container_type &SuccNodes, ESGEdgeKind Kind) override {
    ESG.saveEdges(std::move(Curr), std::move(CurrNode), std::move(Succ),
                  SuccNodes, Kind);
  }

  ExplodedSuperGraph<domain_t> ESG;
};

template <typename ProblemTy>
PathAwareIDESolver(ProblemTy &)
    -> PathAwareIDESolver<typename ProblemTy::ProblemAnalysisDomain>;

} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_SOLVER_PATHAWAREIDESOLVER_H
