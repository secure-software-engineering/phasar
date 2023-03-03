/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"

namespace psr {

void AnalysisController::executeIDEXTaint() {
  auto Config = makeTaintConfig();
  executeIDEAnalysis<IDEExtendedTaintAnalysis<>>(Config, EntryPoints);
}

} // namespace psr
