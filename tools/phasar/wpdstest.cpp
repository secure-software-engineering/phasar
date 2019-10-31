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
#include <phasar/PhasarLLVM/Utils/BinaryDomain.h>
#include <phasar/PhasarLLVM/WPDS/Problems/WPDSLinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/WPDS/Problems/WPDSSolverTest.h>
#include <phasar/PhasarLLVM/WPDS/Solver/LLVMWPDSSolver.h>
#include <phasar/Utils/Logger.h>

#include <llvm/Support/raw_ostream.h>

#include <boost/filesystem/operations.hpp>

using namespace std;
using namespace psr;
namespace bfs = boost::filesystem;

int main(int argc, char **argv) {
  initializeLogger(false);
  auto &lg = lg::get();
  if (argc < 4 || !bfs::exists(argv[1]) || bfs::is_directory(argv[1])) {
    std::cerr << "usage: <prog> <ir file> <ID or LCA> <DIRECTION>\n";
    return 1;
  }
  string DFA(argv[2]);
  if (DFA != "ID" && DFA != "LCA") {
    std::cerr << "analysis is not valid!\n";
    return 1;
  }
  string DIRECTION(argv[3]);
  if (!(DIRECTION == "FORWARD" || DIRECTION == "BACKWARD")) {
    std::cerr << "analysis direction must be FORWARD or BACKWARD\n";
    return 1;
  }
  initializeLogger(false);
  ProjectIRDB DB({argv[1]}, IRDBOptions::WPA);
  DB.preprocessIR();
  const llvm::Function *F;
  if ((F = DB.getFunction("main"))) {
    LLVMTypeHierarchy H(DB);
    LLVMBasedICFG I(H, DB, CallGraphAnalysisType::OTF, {"main"});
    auto Ret = &F->back().back();
    cout << "RESULTS AT: " << llvmIRToString(Ret) << '\n';
    if (DFA == "ID") {
      WPDSSolverTest T(I, H, DB, WPDSType::FWPDS, SearchDirection::FORWARD);
      LLVMWPDSSolver<const llvm::Value *, BinaryDomain, LLVMBasedICFG &> S(T);
      S.solve();
      auto Results = S.resultsAt(Ret);
      cout << "Results:\n";
      for (auto &Result : Results) {
        Result.first->print(llvm::outs());
        cout << '\n';
      }
    } else if (DFA == "LCA") {
      std::cout << "LCA" << std::endl;
      WPDSLinearConstantAnalysis L(I, H, DB, WPDSType::FWPDS, [DIRECTION]() {
        if (DIRECTION == "FORWARD") {
          return SearchDirection::FORWARD;
        }
        return SearchDirection::BACKWARD;
      }());
      LLVMWPDSSolver<const llvm::Value *, int64_t, LLVMBasedICFG &> S(L);
      S.solve();
      auto Results = S.resultsAt(Ret);
      cout << "Results:\n";
      if (!Results.empty()) {
        for (auto &Result : Results) {
          Result.first->print(llvm::outs());
          cout << " - with value: ";
          L.printValue(cout, Result.second);
          std::cout << '\n';
        }
      } else {
        cout << "Results are empty!\n";
      }
    }
    std::cout << "DONE!\n";
  } else {
    std::cerr << "error: file does not contain a 'main' function!\n";
  }
  return 0;
}
