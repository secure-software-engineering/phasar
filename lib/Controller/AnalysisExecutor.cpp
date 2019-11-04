/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include <phasar/Controller/AnalysisExecutor.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/AnalysisStrategy/AnalysisSetup.h>
#include <phasar/PhasarLLVM/AnalysisStrategy/WholeProgramAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoSolverTest.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/LLVMInterMonoSolver.h>

using namespace std;

namespace psr {

void AnalysisExecutor::testExecutor(ProjectIRDB &IRDB) {
  cout << "Testing the novel AnalysisExecutor!\n";
  WholeProgramAnalysis<
      LLVMInterMonoSolver<const llvm::Value *, LLVMBasedICFG &, 3>,
      InterMonoSolverTest>
      WPA(IRDB);
  WPA.solve();
}

} // namespace psr
