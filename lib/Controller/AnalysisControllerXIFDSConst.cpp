/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSConstAnalysis.h"

namespace psr {

void AnalysisController::executeIFDSConst() {
  IFDSConstAnalysis CA(&HA.getProjectIRDB(), &HA.getPointsToInfo(),
                       EntryPoints);
  executeIFDSAnalysis(CA);
}

} // namespace psr
