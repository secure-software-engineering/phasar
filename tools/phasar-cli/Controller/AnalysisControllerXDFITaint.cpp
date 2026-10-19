/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/DFISolver.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"

#include "AnalysisControllerInternalIDE.h"

using namespace psr;

void controller::executeDFITaint(AnalysisController &Data) {
  auto Config = makeTaintConfig(Data);
  // Note: Don't blindly generate argc and argv. Use a proper taint config
  // instead
  auto Problem = createAnalysisProblem<IFDSTaintAnalysis>(
      Data.HA, &Config, Data.getEntryPoints(), false);
  DFISolver Solver(&Problem, &Data.HA.getICFG(), DFISolverConfigNoIndex{},
                   Data.SolverConfig);
  executeIfdsIdeAnalysisImpl(Solver, Data);
}
