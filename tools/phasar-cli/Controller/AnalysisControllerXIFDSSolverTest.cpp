/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSSolverTest.h"

#include "AnalysisControllerInternalIDE.h"

using namespace psr;

void controller::executeIFDSSolverTest(AnalysisController &Data) {
  executeIFDSAnalysis<IFDSSolverTest>(Data, Data.EntryPoints);
}
