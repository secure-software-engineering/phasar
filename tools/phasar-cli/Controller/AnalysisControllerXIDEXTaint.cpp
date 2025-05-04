/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"

#include "AnalysisControllerInternalIDE.h"

using namespace psr;

void controller::executeIDEXTaint(AnalysisController &Data) {
  auto Config = makeTaintConfig(Data);
  executeIDEAnalysis<IDEExtendedTaintAnalysis<>>(Data, Config,
                                                 Data.EntryPoints);
}
