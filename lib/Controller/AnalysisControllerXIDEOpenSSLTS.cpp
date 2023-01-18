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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h"

namespace psr {

void AnalysisController::executeIDEOpenSSLTS() {
  OpenSSLEVPKDFDescription TSDesc;
  IDETypeStateAnalysis TSA(&HA.getProjectIRDB(), &HA.getAliasInfo(), &TSDesc,
                           EntryPoints);
  executeIDEAnalysis(TSA);
}

} // namespace psr
