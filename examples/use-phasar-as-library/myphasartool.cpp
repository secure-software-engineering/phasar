/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <fstream>
#include <iostream>

#include "boost/filesystem/operations.hpp"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/WholeProgramAnalysis.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSLinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"

namespace llvm {
class Value;
} // namespace llvm

using namespace psr;

int main(int argc, const char **argv) {
  if (argc < 2 || !boost::filesystem::exists(argv[1]) ||
      boost::filesystem::is_directory(argv[1])) {
    std::cerr << "myphasartool\n"
                 "A small PhASAR-based example program\n\n"
                 "Usage: myphasartool <LLVM IR file>\n";
    return 1;
  }
  initializeLogger(false);
  ProjectIRDB DB({argv[1]});
  if (auto F = DB.getFunctionDefinition("main")) {
    LLVMTypeHierarchy H(DB);
    // print type hierarchy
    H.print();
    LLVMPointsToSet P(DB);
    // print points-to information
    P.print();
    LLVMBasedICFG I(DB, CallGraphAnalysisType::OTF, {"main"}, &H, &P);
    // print inter-procedural control-flow graph
    I.print();
    // IFDS template parametrization test
    std::cout << "Testing IFDS:\n";
    IFDSLinearConstantAnalysis L(&DB, &H, &I, &P, {"main"});
    IFDSSolver_P<IFDSLinearConstantAnalysis> S(L);
    S.solve();
    S.dumpResults();
    // use PhASAR's strategy concept that allows for even easier analysis set-up
    std::cout << "Testing IDE:\n";
    WholeProgramAnalysis<IDESolver_P<IDELinearConstantAnalysis>,
                         IDELinearConstantAnalysis>
        WPA(DB, {"main"}, &P, &I, &H);
    WPA.solve();
    WPA.dumpResults();
    WPA.releaseAllHelperAnalyses();
  } else {
    std::cerr << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
