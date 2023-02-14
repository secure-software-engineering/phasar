/******************************************************************************
 * Copyright (c) 2022 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#include "phasar/Controller/AnalysisController.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoSolverTest.h"

namespace psr {

void AnalysisController::executeInterMonoSolverTest() {
  InterMonoSolverTest ST(&HA.getProjectIRDB(), &HA.getTypeHierarchy(),
                         &HA.getICFG(), &HA.getAliasInfo(), EntryPoints);

  executeInterMonoAnalysis(ST);
}

} // namespace psr
