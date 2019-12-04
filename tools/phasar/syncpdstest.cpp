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
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/DataFlowSolver/SyncPDS/Solver/SyncPDSSolver.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

int main(int argc, char **argv) {
  initializeLogger(false);
  auto &lg = lg::get();
  if (argc < 2 || !boost::filesystem::exists(argv[1]) || boost::filesystem::is_directory(argv[1])) {
    std::cerr << "usage: <prog> <ir file>\n";
    std::cerr << "use programs in build/test/llvm_test_code/fields/\n";
    return 1;
  }
  initializeLogger(false);
  ProjectIRDB DB({argv[1]}, IRDBOptions::WPA);
  LLVMTypeHierarchy H(DB);
  LLVMBasedICFG ICF(H, DB, CallGraphAnalysisType::OTF, {"main"});
  const llvm::Function *F = nullptr;
  if (!(F = ICF.getFunction("main"))) {
    std::cerr << "program does not contain a 'main' function!\n";
    return 1;
  }

  return 0;
}
