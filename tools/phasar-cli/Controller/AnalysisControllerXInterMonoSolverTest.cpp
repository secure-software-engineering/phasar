/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/Mono/Problems/InterMonoSolverTest.h"

#include "AnalysisControllerInternalMono.h"

using namespace psr;

void controller::executeInterMonoSolverTest(AnalysisController &Data) {
  executeInterMonoAnalysis<InterMonoSolverTest>(Data, Data.EntryPoints);
}
