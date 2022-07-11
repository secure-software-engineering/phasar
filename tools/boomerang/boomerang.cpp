/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <filesystem>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/SyncPDS/Solver/SyncPDSSolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"

#include "llvm/Support/raw_ostream.h"

using namespace psr;

int main(int Argc, char **Argv) {
  if (Argc < 2 || !std::filesystem::exists(Argv[1]) ||
      std::filesystem::is_directory(Argv[1])) {
    llvm::errs() << "usage: <prog> <ir file>\n";
    llvm::errs() << "use programs in build/test/llvm_test_code/pointers/\n";
    return 1;
  }
  ProjectIRDB DB({Argv[1]}, IRDBOptions::WPA);
  LLVMTypeHierarchy H(DB);
  LLVMPointsToSet P(DB);
  LLVMBasedICFG ICFG(DB, CallGraphAnalysisType::OTF, {"main"}, &H, &P);
  for (auto &F : *DB.getWPAModule()) {
    if (F.isDeclaration()) {
      continue;
    }
    llvm::outs() << "ANALYZE FUNCTION: " << F.getName() << '\n';
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *Store = llvm::dyn_cast<llvm::StoreInst>(&I)) {
          // direct store to memory location
          if (auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(
                  Store->getPointerOperand())) {
            llvm::outs() << "Found trivial store at: ";
            Store->print(llvm::outs());
            llvm::outs() << " --- to: ";
            Alloca->print(llvm::outs());
            llvm::outs() << '\n';
            // indirect store to a loaded value
          } else if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(
                         Store->getPointerOperand())) {
            llvm::outs() << "Found non-trivial store at: ";
            Store->print(llvm::outs());
            llvm::outs() << " --- need to find aliases of: ";
            Load->getPointerOperand()->print(llvm::outs());
            llvm::outs() << '\n';
            // query SPDS solver to find the aliases
            // SyncPDSSolver SPDS(ICFG);
            // set<const llvm::Value *> Aliases =
            // SPDS.getAliasesOf(Load->getPointerOperand()); llvm::outs() <<
            // "Found aliases:"; for (auto A : Aliases) {
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
