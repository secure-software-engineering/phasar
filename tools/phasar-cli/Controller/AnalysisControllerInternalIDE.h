/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLLER_ANALYSISCONTROLLERINTERNALIDE_H
#define PHASAR_CONTROLLER_ANALYSISCONTROLLERINTERNALIDE_H

#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"

#include "AnalysisControllerInternal.h"

namespace psr::controller {

template <typename T, typename U>
static void statsEmitter(llvm::raw_ostream &OS, const IDESolver<T, U> &Solver) {
  OS << "\nEdgeFunction Statistics:\n";
  Solver.printEdgeFunctionStatistics(OS);
}

template <typename SolverTy, typename ProblemTy, typename... ArgTys>
static void executeIfdsIdeAnalysis(AnalysisController &Data, ArgTys &&...Args) {
  auto Problem =
      createAnalysisProblem<ProblemTy>(*Data.HA, std::forward<ArgTys>(Args)...);
  SolverTy Solver(Problem, &Data.HA->getICFG());
  {
    std::optional<Timer> MeasureTime;
    if (Data.EmitterOptions &
        AnalysisControllerEmitterOptions::EmitStatisticsAsText) {
      MeasureTime.emplace([](auto Elapsed) {
        llvm::outs() << "Elapsed: " << hms{Elapsed} << '\n';
      });
    }

    Solver.solve();
  }
  emitRequestedDataFlowResults(Data, Solver);
}

template <typename ProblemTy, typename... ArgTys>
static void executeIFDSAnalysis(AnalysisController &Data, ArgTys &&...Args) {
  executeIfdsIdeAnalysis<IFDSSolver_P<ProblemTy>, ProblemTy>(
      Data, std::forward<ArgTys>(Args)...);
}

template <typename ProblemTy, typename... ArgTys>
static void executeIDEAnalysis(AnalysisController &Data, ArgTys &&...Args) {
  executeIfdsIdeAnalysis<IDESolver_P<ProblemTy>, ProblemTy>(
      Data, std::forward<ArgTys>(Args)...);
}

} // namespace psr::controller

#endif // PHASAR_CONTROLLER_ANALYSISCONTROLLERINTERNALMONO_H
