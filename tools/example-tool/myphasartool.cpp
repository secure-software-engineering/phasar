/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

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
    std::cerr << "usage: <prog> <ir file>\n";
    return 1;
  }
  initializeLogger(false);
  ProjectIRDB DB({argv[1]});
  if (auto F = DB.getFunctionDefinition("main")) {
    LLVMTypeHierarchy H(DB);
    std::cout << "LLVMPointstoInfo:\n";
    LLVMPointsToInfo PT(DB);
    // auto PTG = PT.getPointsToGraph(F);
    // PTG->print();
    std::cout << "LLVMBasedICFG:\n";
    LLVMBasedICFG I(DB, CallGraphAnalysisType::OTF, {"main"}, &H, &PT);
    // I.print();
    // std::cout << "=== Call graph ===\n";
    // I.print();
    // I.printAsDot("call_graph.dot");
    // // IFDS template parametrization test
    // IFDSLinearConstantAnalysis L(&DB, &H, &I, &PT, {"main"});
    // IFDSSolver<IFDSLinearConstantAnalysis::n_t, IFDSLinearConstantAnalysis::d_t,
    //            IFDSLinearConstantAnalysis::m_t, IFDSLinearConstantAnalysis::t_t,
    //            IFDSLinearConstantAnalysis::v_t, IFDSLinearConstantAnalysis::i_t>
    //     S(L);
    // S.solve();
    // S.dumpResults();
    // // IDE template parametrization test
    // IDELinearConstantAnalysis M(&DB, &H, &I, &PT, {"main"});
    // IDESolver<IDELinearConstantAnalysis::n_t, IDELinearConstantAnalysis::d_t,
    //           IDELinearConstantAnalysis::m_t, IDELinearConstantAnalysis::t_t,
    //           IDELinearConstantAnalysis::v_t, IDELinearConstantAnalysis::l_t,
    //           IDELinearConstantAnalysis::i_t>
    //     T(M);
    // T.solve();
    // T.dumpResults();
  } else {
    std::cerr << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
