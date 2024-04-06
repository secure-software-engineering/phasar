/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDESolverTest.h"

#include "AnalysisControllerInternalIDE.h"

using namespace psr;

void controller::executeIDESolverTest(AnalysisController &Data) {
  executeIDEAnalysis<IDESolverTest>(Data, Data.EntryPoints);
}
