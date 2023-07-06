/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_ANALYSISSTRATEGY_VARIATIONALANALYSIS_H
#define PHASAR_ANALYSISSTRATEGY_VARIATIONALANALYSIS_H

#include "phasar/AnalysisStrategy/AnalysisSetup.h"

#include <type_traits>

namespace psr {

template <typename Solver, typename ProblemDescription,
          typename Setup = psr::DefaultAnalysisSetup>
class VariationalAnalysis {
  static_assert(
      std::is_base_of_v<typename Solver::ProblemType, ProblemDescription>,
      "ProblemDesciption does not match SolverType");

private:
public:
};

} // namespace psr

#endif
