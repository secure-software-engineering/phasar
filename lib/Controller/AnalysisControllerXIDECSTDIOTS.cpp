/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h"

namespace psr {

void AnalysisController::executeIDECSTDIOTS() {
  CSTDFILEIOTypeStateDescription TSDesc;
  IDETypeStateAnalysis TSA(&HA.getProjectIRDB(), &HA.getPointsToInfo(), &TSDesc,
                           EntryPoints);

  executeIDEAnalysis(TSA);
}

} // namespace psr
