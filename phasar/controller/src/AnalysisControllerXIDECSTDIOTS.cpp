/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h"

namespace psr {

void AnalysisController::executeIDECSTDIOTS() {
  CSTDFILEIOTypeStateDescription TSDesc;
  WholeProgramAnalysis<IDESolver_P<IDETypeStateAnalysis>, IDETypeStateAnalysis>
      WPA(SolverConfig, IRDB, &TSDesc, EntryPoints, &PT, &ICF, &TH);
  WPA.solve();
  emitRequestedDataFlowResults(WPA);
  WPA.releaseAllHelperAnalyses();
}

} // namespace psr
