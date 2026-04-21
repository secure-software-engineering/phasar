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
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedICFGView.h"

#include "AnalysisControllerInternal.h"

namespace psr::controller {

template <typename T, typename U, typename I>
static void statsEmitter(llvm::raw_ostream &OS,
                         const IDESolver<T, U, I> &Solver) {
  OS << "\nEdgeFunction Statistics:\n";
  Solver.printEdgeFunctionStatistics(OS);
}

template <typename SolverTy>
static void executeIfdsIdeAnalysisImpl(SolverTy &Solver,
                                       AnalysisController &Data) {
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

template <typename SolverTy, typename ProblemTy, typename... ArgTys>
static void executeSparseIfdsIdeAnalysis(AnalysisController &Data,
                                         ArgTys &&...Args) {
  SparseLLVMBasedICFGView SVFG(&Data.HA->getICFG(), Data.HA->getAliasInfo());
  executeIfdsIdeAnalysisImpl<SolverTy, ProblemTy>(
      Data, SVFG, std::forward<ArgTys>(Args)...);
}

template <typename ProblemTy, typename... ArgTys>
static void executeIFDSAnalysisWithICFG(AnalysisController &Data,
                                        const ICFG auto &ICF,
                                        ArgTys &&...Args) {
  auto Problem =
      createAnalysisProblem<ProblemTy>(*Data.HA, std::forward<ArgTys>(Args)...);
  IFDSSolver Solver(&Problem, &ICF);
  executeIfdsIdeAnalysisImpl(Solver, Data);
}
template <typename ProblemTy, typename... ArgTys>
static void executeIDEAnalysisWithICFG(AnalysisController &Data,
                                       const ICFG auto &ICF, ArgTys &&...Args) {
  auto Problem =
      createAnalysisProblem<ProblemTy>(*Data.HA, std::forward<ArgTys>(Args)...);
  IDESolver Solver(&Problem, &ICF);
  executeIfdsIdeAnalysisImpl(Solver, Data);
}

template <typename ProblemTy, typename... ArgTys>
static void executeIFDSAnalysis(AnalysisController &Data, ArgTys &&...Args) {
  executeIFDSAnalysisWithICFG<ProblemTy>(Data, Data.HA->getICFG(),
                                         PSR_FWD(Args)...);
}

template <typename ProblemTy, typename... ArgTys>
static void executeSparseIFDSAnalysis(AnalysisController &Data,
                                      ArgTys &&...Args) {
  SparseLLVMBasedICFGView SVFG(&Data.HA->getICFG(), Data.HA->getAliasInfo());
  executeIFDSAnalysisWithICFG<ProblemTy>(Data, SVFG, PSR_FWD(Args)...);
}

template <typename ProblemTy, typename... ArgTys>
static void executeIDEAnalysis(AnalysisController &Data, ArgTys &&...Args) {
  executeIDEAnalysisWithICFG<ProblemTy>(Data, Data.HA->getICFG(),
                                        PSR_FWD(Args)...);
}

template <typename ProblemTy, typename... ArgTys>
static void executeSparseIDEAnalysis(AnalysisController &Data,
                                     ArgTys &&...Args) {
  SparseLLVMBasedICFGView SVFG(&Data.HA->getICFG(), Data.HA->getAliasInfo());
  executeIDEAnalysisWithICFG<ProblemTy>(Data, SVFG, PSR_FWD(Args)...);
}

} // namespace psr::controller

#endif // PHASAR_CONTROLLER_ANALYSISCONTROLLERINTERNALMONO_H
