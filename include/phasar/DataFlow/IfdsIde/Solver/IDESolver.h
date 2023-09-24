/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IDESolver.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_IDESOLVER_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_IDESOLVER_H

#include "phasar/DataFlow/IfdsIde/Solver/detail/IDESolverImpl.h"

namespace psr {

/// Solves the given IDETabulationProblem as described in the 1996 paper by
/// Sagiv, Horwitz and Reps. To solve the problem, call solve(). Results
/// can then be queried by using resultAt() and resultsAt().
///
/// Propagates data-flow facts to the successors of the statement, where they
/// were generated.
template <typename AnalysisDomainTy, typename Container>
class IDESolver<AnalysisDomainTy, Container, PropagateOverStrategy>
    : public IDESolverImpl<
          IDESolver<AnalysisDomainTy, Container, PropagateOverStrategy>,
          AnalysisDomainTy, Container, PropagateOverStrategy> {
  using base_t = IDESolverImpl<
      IDESolver<AnalysisDomainTy, Container, PropagateOverStrategy>,
      AnalysisDomainTy, Container, PropagateOverStrategy>;

public:
  using ProblemTy = IDETabulationProblem<AnalysisDomainTy, Container>;
  using container_type = typename ProblemTy::container_type;
  using FlowFunctionPtrType = typename ProblemTy::FlowFunctionPtrType;

  using l_t = typename AnalysisDomainTy::l_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;

  explicit IDESolver(IDETabulationProblem<AnalysisDomainTy, Container> &Problem,
                     const i_t *ICF, PropagateOverStrategy Strategy = {})
      : base_t(Problem, ICF, Strategy) {}

private:
  friend base_t;

  std::vector<PathEdge<n_t, d_t>> WorkList;
};

template <typename AnalysisDomainTy, typename Container>
OwningSolverResults<typename AnalysisDomainTy::n_t,
                    typename AnalysisDomainTy::d_t,
                    typename AnalysisDomainTy::l_t>
solveIDEProblem(IDETabulationProblem<AnalysisDomainTy, Container> &Problem,
                const typename AnalysisDomainTy::i_t &ICF,
                PropagateOverStrategy Strategy = {}) {
  IDESolver Solver(Problem, &ICF, Strategy);
  Solver.solve();
  return Solver.consumeSolverResults();
}

} // namespace psr

#endif
