/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"

namespace psr {

void AnalysisController::executeIFDSTaint() {
  auto Config = makeTaintConfig();
  executeIFDSAnalysis<IFDSTaintAnalysis>(&Config, EntryPoints);
}

} // namespace psr
