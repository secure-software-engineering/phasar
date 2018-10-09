/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/WPDS/Problems/WPDSLinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/WPDS/Problems/WPDSSolverTest.h>
#include <phasar/PhasarLLVM/WPDS/Solver/LLVMWPDSSolver.h>
#include <phasar/Utils/Logger.h>

#include <llvm/Support/raw_ostream.h>

#include <boost/filesystem/operations.hpp>

using namespace std;
using namespace psr;

int main(int argc, char **argv) {
  cout << "Hello, WPDS-Test!\n";
  initializeLogger(false);
  auto &lg = lg::get();
  if (argc < 2 || !bfs::exists(argv[1]) || bfs::is_directory(argv[1])) {
    std::cerr << "usage: <prog> <ir file>\n";
    return 1;
  }
  initializeLogger(false);
  ProjectIRDB DB({argv[1]}, IRDBOptions::WPA);
  DB.preprocessIR();
  const llvm::Function *F;
  if ((F = DB.getFunction("main"))) {
    LLVMTypeHierarchy H(DB);
    LLVMBasedICFG I(H, DB, CallGraphAnalysisType::OTF, {"main"});
    std::cout << "=== Call graph ===\n";
    I.print();
    WPDSLinearConstantAnalysis L(I, WPDSType::FWPDS, SearchDirection::FORWARD);
    WPDSSolverTest T(I, WPDSType::FWPDS, SearchDirection::FORWARD);
    // LLVMWPDSSolver<const llvm::Value *, int64_t, LLVMBasedICFG &> S(L);
    LLVMWPDSSolver<const llvm::Value *, bool, LLVMBasedICFG &> S(T);
    auto Return = &F->back().back();
    Return->print(llvm::outs()); llvm::outs() << '\n';
    auto Solution = S.solve(Return);
    if (auto EnvTrafo = dynamic_cast<EnvTrafoToSemElem<bool> *>(&(*Solution))) {
      std::cout << "Solution is: " << *EnvTrafo << '\n';
    }
  } else {
    std::cerr << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
