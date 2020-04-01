/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/DataFlowSolver/SyncPDS/Solver/SyncPDSSolver.h"
#include "phasar/Utils/Logger.h"

#include "llvm/Support/raw_ostream.h"

#include "boost/filesystem/operations.hpp"

using namespace std;
using namespace psr;
namespace bfs = boost::filesystem;

int main(int argc, char **argv) {
  initializeLogger(false);
  auto &lg = lg::get();
  if (argc < 2 || !bfs::exists(argv[1]) || bfs::is_directory(argv[1])) {
    std::cerr << "usage: <prog> <ir file>\n";
    std::cerr << "use programs in build/test/llvm_test_code/pointers/\n";
    return 1;
  }
  initializeLogger(false);
  ProjectIRDB DB({argv[1]}, IRDBOptions::WPA);
  LLVMTypeHierarchy H(DB);
  LLVMPointsToInfo P(DB);
  LLVMBasedICFG ICFG(DB, CallGraphAnalysisType::OTF, {"main"}, &H, &P);
  for (auto &F : *DB.getWPAModule()) {
    if (F.isDeclaration()) { continue; }
    llvm::outs() << "ANALYZE FUNCTION: " << F.getName() << '\n';
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto Store = llvm::dyn_cast<llvm::StoreInst>(&I)) {
          // direct store to memory location
          if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(
                  Store->getPointerOperand())) {
            llvm::outs() << "Found trivial store at: ";
            Store->print(llvm::outs());
            llvm::outs() << " --- to: ";
            Alloca->print(llvm::outs());
            llvm::outs() << '\n';
            // indirect store to a loaded value
          } else if (auto Load = llvm::dyn_cast<llvm::LoadInst>(
                         Store->getPointerOperand())) {
            llvm::outs() << "Found non-trivial store at: ";
            Store->print(llvm::outs());
            llvm::outs() << " --- need to find aliases of: ";
            Load->getPointerOperand()->print(llvm::outs());
            llvm::outs() << '\n';
            // query SPDS solver to find the aliases
            // SyncPDSSolver SPDS(ICFG);
            // set<const llvm::Value *> Aliases = SPDS.getAliasesOf(Load->getPointerOperand());
            // llvm::outs() << "Found aliases:";
            // for (auto A : Aliases) {
            //   A->print(llvm::outs() << '\n');
            // }
            llvm::outs() << '\n';
          } else {
            llvm::outs() << "Ups!\n";
          }
        }
      }
    }
  }
  return 0;
}
