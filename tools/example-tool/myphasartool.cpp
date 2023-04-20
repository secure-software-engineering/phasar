/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include <filesystem>
#include <string>

using namespace psr;

int main(int Argc, const char **Argv) {
  using namespace std::string_literals;

  if (Argc < 2 || !std::filesystem::exists(Argv[1]) ||
      std::filesystem::is_directory(Argv[1])) {
    llvm::errs() << "myphasartool\n"
                    "A small PhASAR-based example program\n\n"
                    "Usage: myphasartool <LLVM IR file>\n";
    return 1;
  }

  std::vector EntryPoints = {"main"s};

  HelperAnalyses HA(Argv[1], EntryPoints);

  if (const auto *F = HA.getProjectIRDB().getFunctionDefinition("main")) {
    // print type hierarchy
    HA.getTypeHierarchy().print();
    // print points-to information
    HA.getAliasInfo().print();
    // print inter-procedural control-flow graph
    HA.getICFG().print();

    // IFDS template parametrization test
    llvm::outs() << "Testing IFDS:\n";
    auto L = createAnalysisProblem<IFDSSolverTest>(HA, EntryPoints);
    IFDSSolver S(L, &HA.getICFG());
    S.solve();
    S.dumpResults();
    // IDE template parametrization test
    llvm::outs() << "Testing IDE:\n";
    auto M = createAnalysisProblem<IDELinearConstantAnalysis>(HA, EntryPoints);

    // Alternative way of solving an IFDS/IDEProblem:
    auto IDEResults = solveIDEProblem(M, HA.getICFG());
    IDEResults.dumpResults(HA.getICFG(), M);

  } else {
    llvm::errs() << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
