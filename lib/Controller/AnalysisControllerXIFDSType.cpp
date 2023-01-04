/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTypeAnalysis.h"

namespace psr {

void AnalysisController::executeIFDSType() {
  IFDSTypeAnalysis TA(&HA.getProjectIRDB(), EntryPoints);
  executeIFDSAnalysis(TA);
}

} // namespace psr
