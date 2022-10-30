/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEInstInteractionAnalysis.h"

namespace psr {

void AnalysisController::executeIDEIIA() {
  IDEInstInteractionAnalysis IIA(&HA.getProjectIRDB(), &HA.getICFG(),
                                 &HA.getPointsToInfo(), EntryPoints);
  executeIDEAnalysis(IIA);
}

} // namespace psr
