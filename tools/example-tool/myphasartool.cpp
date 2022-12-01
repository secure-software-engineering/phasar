/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/HelperAnalyses.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"

#include <filesystem>

using namespace psr;

int main(int Argc, const char **Argv) {
  if (Argc < 2 || !std::filesystem::exists(Argv[1]) ||
      std::filesystem::is_directory(Argv[1])) {
    llvm::errs() << "myphasartool\n"
                    "A small PhASAR-based example program\n\n"
                    "Usage: myphasartool <LLVM IR file>\n";
    return 1;
  }

  HelperAnalyses HA({Argv[1]}, {"main"});

  if (const auto *F = HA.getProjectIRDB().getFunctionDefinition("main")) {
    // print type hierarchy
    HA.getTypeHierarchy().print();
    // print points-to information
    HA.getPointsToInfo().print();
    // print inter-procedural control-flow graph
    HA.getICFG().print();

    // IFDS template parametrization test
    llvm::outs() << "Testing IFDS:\n";
    IFDSSolverTest L(&HA.getProjectIRDB(), {"main"});
    IFDSSolver S(L, &HA.getICFG());
    S.solve();
    S.dumpResults();
    // IDE template parametrization test
    llvm::outs() << "Testing IDE:\n";
    IDELinearConstantAnalysis M(&HA.getProjectIRDB(), &HA.getICFG(), {"main"});
    IDESolver T(M, &HA.getICFG());
    T.solve();
    T.dumpResults();
  } else {
    llvm::errs() << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
