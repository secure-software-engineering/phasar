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
#include <phasar/PhasarLLVM/SyncPDS/Solver/SyncPDSSolver.h>
#include <phasar/Utils/Logger.h>

#include <llvm/Support/raw_ostream.h>

#include <boost/filesystem/operations.hpp>

using namespace std;
using namespace psr;
namespace bfs = boost::filesystem;

int main(int argc, char **argv) {
  initializeLogger(false);
  auto &lg = lg::get();
  if (argc < 2 || !bfs::exists(argv[1]) || bfs::is_directory(argv[1])) {
    std::cerr << "usage: <prog> <ir file>\n";
    std::cerr << "use programs in build/test/llvm_test_code/fields/\n";
    return 1;
  }
  initializeLogger(false);
  ProjectIRDB DB({argv[1]}, IRDBOptions::WPA);
  DB.preprocessIR();
  LLVMTypeHierarchy H(DB);
  LLVMBasedICFG ICFG(H, DB, CallGraphAnalysisType::OTF, {"main"});
  const llvm::Function *F = nullptr;
  if (!(F = ICFG.getMethod("main"))) {
    std::cerr << "program does not contain a 'main' function!\n";
    return 1;
  }

  return 0;
}
