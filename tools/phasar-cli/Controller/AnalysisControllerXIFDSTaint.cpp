/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"

#include "AnalysisControllerInternalIDE.h"

using namespace psr;

void controller::executeIFDSTaint(AnalysisController &Data) {
  auto Config = makeTaintConfig(Data);
  executeIFDSAnalysis<IFDSTaintAnalysis>(Data, &Config, Data.EntryPoints);
}
