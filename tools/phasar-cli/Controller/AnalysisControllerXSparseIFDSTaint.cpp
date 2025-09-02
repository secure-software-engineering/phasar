/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"

#include "AnalysisControllerInternalIDE.h"

using namespace psr;

void controller::executeSparseIFDSTaint(AnalysisController &Data) {
  auto Config = makeTaintConfig(Data);
  executeSparseIFDSAnalysis<IFDSTaintAnalysis>(Data, &Config, Data.EntryPoints);
}
