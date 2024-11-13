/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLLER_ANALYSISCONTROLLERINTERNALMONO_H
#define PHASAR_CONTROLLER_ANALYSISCONTROLLERINTERNALMONO_H

#include "phasar/DataFlow/Mono/Solver/InterMonoSolver.h"
#include "phasar/DataFlow/Mono/Solver/IntraMonoSolver.h"

#include "AnalysisControllerInternal.h"

namespace psr::controller {

template <typename SolverTy, typename ProblemTy, typename... ArgTys>
static void executeMonoAnalysis(AnalysisController &Data, ArgTys &&...Args) {
  auto Problem =
      createAnalysisProblem<ProblemTy>(*Data.HA, std::forward<ArgTys>(Args)...);
  SolverTy Solver(Problem);
  Solver.solve();
  emitRequestedDataFlowResults(Data, Solver);
}

template <typename ProblemTy, typename... ArgTys>
static void executeIntraMonoAnalysis(AnalysisController &Data,
                                     ArgTys &&...Args) {
  executeMonoAnalysis<IntraMonoSolver_P<ProblemTy>, ProblemTy>(
      Data, std::forward<ArgTys>(Args)...);
}

template <typename ProblemTy, typename... ArgTys>
static void executeInterMonoAnalysis(AnalysisController &Data,
                                     ArgTys &&...Args) {
  executeMonoAnalysis<InterMonoSolver_P<ProblemTy, K>, ProblemTy>(
      Data, std::forward<ArgTys>(Args)...);
}
} // namespace psr::controller

#endif // PHASAR_CONTROLLER_ANALYSISCONTROLLERINTERNALMONO_H
