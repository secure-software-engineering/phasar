/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/Mono/Problems/IntraMonoFullConstantPropagation.h"

#include "AnalysisControllerInternalMono.h"

using namespace psr;

void controller::executeIntraMonoFullConstant(AnalysisController &Data) {
  executeIntraMonoAnalysis<IntraMonoFullConstantPropagation>(Data,
                                                             Data.EntryPoints);
}
