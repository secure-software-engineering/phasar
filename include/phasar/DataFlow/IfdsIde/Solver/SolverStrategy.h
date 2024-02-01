/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_SOLVERSTRATEGY_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_SOLVERSTRATEGY_H

namespace psr {

enum class SolverStrategyKind {
  /// Propagate the data-flow effects of an instruction to the start of the
  /// successor instructions. This is the default strategy
  PropagateOver,
  // Propagate the data-flow effects of an instruction onto the same
  // instruction.
  PropagateOnto,
};

struct SolverStrategy {};

struct PropagateOverStrategy : SolverStrategy {
  static constexpr auto Kind = SolverStrategyKind::PropagateOver;
};

struct PropagateOntoStrategy : SolverStrategy {
  static constexpr auto Kind = SolverStrategyKind::PropagateOnto;
};

template <typename AnalysisDomainTy, typename Container,
          typename Strategy = PropagateOverStrategy>
class IDESolver;

template <typename Problem, typename ICF>
IDESolver(Problem &, ICF *)
    -> IDESolver<typename Problem::ProblemAnalysisDomain,
                 typename Problem::container_type, PropagateOverStrategy>;
template <typename Problem, typename ICF>
IDESolver(Problem &, ICF *, PropagateOverStrategy)
    -> IDESolver<typename Problem::ProblemAnalysisDomain,
                 typename Problem::container_type, PropagateOverStrategy>;

template <typename Problem, typename Strategy = PropagateOverStrategy>
using IDESolver_P = IDESolver<typename Problem::ProblemAnalysisDomain,
                              typename Problem::container_type, Strategy>;

} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_SOLVER_SOLVERSTRATEGY_H
