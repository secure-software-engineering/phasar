/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <llvm/IR/Value.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSLinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIFDSSolver.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIDESolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/Logger.h>
#include <boost/filesystem/operations.hpp>

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

using namespace std;
using namespace psr;

int main(int argc, const char **argv) {
  initializeLogger(false);
  auto &lg = lg::get();
  if (argc < 2 || !bfs::exists(argv[1]) || bfs::is_directory(argv[1])) {
    std::cerr << "usage: <prog> <ir file>\n";
    return 1;
  }
  initializeLogger(false);
  ProjectIRDB DB({argv[1]}, IRDBOptions::WPA);
  DB.preprocessIR();
  if (DB.getFunction("main")) {
    LLVMTypeHierarchy H(DB);
    LLVMBasedICFG I(H, DB, CallGraphAnalysisType::OTF,
                    {"main"});
    std::cout << "=== Call graph ===\n";
    I.print();
    I.printAsDot("call_graph.dot");
    // IFDS template parametrization test
		IFDSLinearConstantAnalysis L(I, {"main"});
    LLVMIFDSSolver<LCAPair, LLVMBasedICFG &> S(L, true);
    S.solve();
		// IDE template parametrization test
		IDELinearConstantAnalysis M(I, {"main"});
		LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> T(M, true);
		T.solve();
  } else {
    std::cerr << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
