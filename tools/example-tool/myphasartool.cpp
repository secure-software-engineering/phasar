/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>
#include <fstream>

#include <boost/filesystem/operations.hpp>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSLinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/Logger.h>

namespace llvm {
class Value;
} // namespace llvm

using namespace psr;

int main(int argc, const char **argv) {
  initializeLogger(false);
  auto &lg = lg::get();
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
    LLVMPointsToInfo P(DB);
    // print points-to information
    P.print();
    LLVMBasedICFG I(DB, CallGraphAnalysisType::OTF, {"main"}, &H, &P);
    // print inter-procedural control-flow graph
    I.print();
    // IFDS template parametrization test
    std::cout << "Testing IFDS:\n";
    IFDSLinearConstantAnalysis L(&DB, &H, &I, &P, {"main"});
    IFDSSolver<IFDSLinearConstantAnalysis::n_t, IFDSLinearConstantAnalysis::d_t,
               IFDSLinearConstantAnalysis::m_t, IFDSLinearConstantAnalysis::t_t,
               IFDSLinearConstantAnalysis::v_t, IFDSLinearConstantAnalysis::i_t>
        S(L);
    S.solve();
    S.dumpResults();
    // IDE template parametrization test
    std::cout << "Testing IDE:\n";
    IDELinearConstantAnalysis M(&DB, &H, &I, &P, {"main"});
    IDESolver<IDELinearConstantAnalysis::n_t, IDELinearConstantAnalysis::d_t,
              IDELinearConstantAnalysis::m_t, IDELinearConstantAnalysis::t_t,
              IDELinearConstantAnalysis::v_t, IDELinearConstantAnalysis::l_t,
              IDELinearConstantAnalysis::i_t>
        T(M);
    T.solve();
    T.dumpResults();
  } else {
    std::cerr << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
