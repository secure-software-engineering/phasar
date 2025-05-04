/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTypeAnalysis.h"

#include "AnalysisControllerInternalIDE.h"

using namespace psr;

void controller::executeIFDSType(AnalysisController &Data) {
  executeIFDSAnalysis<IFDSTypeAnalysis>(Data, Data.EntryPoints);
}
