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
#include <phasar/PhasarLLVM/SPDS/Solver/SPDSSolver.h>
#include <phasar/Utils/Logger.h>

#include <llvm/Support/raw_ostream.h>

#include <boost/filesystem/operations.hpp>

using namespace std;
using namespace psr;

int main(int argc, char **argv) {
  initializeLogger(false);
  auto &lg = lg::get();
  if (argc < 2 || !bfs::exists(argv[1]) || bfs::is_directory(argv[1])) {
    std::cerr << "usage: <prog> <ir file>\n";
    std::cerr << "use programs in test/llvm_test_code/pointers/";
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
    for (auto &BB : *F) {
      for (auto &I : BB) {
        if (auto Store = llvm::dyn_cast<llvm::StoreInst>(&I)) {
          if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(
                  Store->getPointerOperand())) {
            llvm::outs() << "Found trivial store at: ";
            Store->print(llvm::outs());
            llvm::outs() << " --- to: ";
            Alloca->print(llvm::outs());
            llvm::outs() << '\n';
          } else if (auto Load = llvm::dyn_cast<llvm::LoadInst>(
                         Store->getPointerOperand())) {
            llvm::outs() << "Found non-trivial store at: ";
            Store->print(llvm::outs());
            llvm::outs() << " --- need to find aliases of: ";
            Load->getPointerOperand()->print(llvm::outs());
            llvm::outs() << '\n';
            SPDSSolver SPDS;
            // TODO query SPDS solver
          } else {
            llvm::outs() << "Ups!\n";
          }
        }
      }
    }
  } else {
    std::cerr << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
