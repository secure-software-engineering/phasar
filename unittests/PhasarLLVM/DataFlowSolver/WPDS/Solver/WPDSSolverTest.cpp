/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/
#include "gtest/gtest.h"

#include <iostream>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSLinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Solver/WPDSSolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"
#include "phasar/Utils/Logger.h"

#include "llvm/Support/raw_ostream.h"

#include "boost/filesystem/operations.hpp"

using namespace std;
using namespace psr;

// int main(int argc, char **argv) {
//   initializeLogger(false);
//   if (argc < 4 || !bfs::exists(argv[1]) || bfs::is_directory(argv[1])) {
//     std::cerr << "usage: <prog> <ir file> <ID or LCA> <DIRECTION>\n";
//     return 1;
//   }
//   string DFA(argv[2]);
//   if (DFA != "ID" && DFA != "LCA") {
//     std::cerr << "analysis is not valid!\n";
//     return 1;
//   }
//   string DIRECTION(argv[3]);
//   if (!(DIRECTION == "FORWARD" || DIRECTION == "BACKWARD")) {
//     std::cerr << "analysis direction must be FORWARD or BACKWARD\n";
//     return 1;
//   }
//   initializeLogger(false);
//   ProjectIRDB DB({argv[1]});
//   const llvm::Function *F;
//   if ((F = DB.getFunctionDefinition("main"))) {
//     LLVMTypeHierarchy H(DB);
//     LLVMPointsToInfo PT(DB);
//     LLVMBasedICFG I(H, DB, CallGraphAnalysisType::OTF, {"main"});
//     auto Ret = &F->back().back();
//     cout << "RESULTS AT: " << llvmIRToString(Ret) << '\n';
//     if (DFA == "ID") {
//       WPDSSolverTest T(&DB, &H, &I, &PT, {"main"});
//       WPDSSolver<WPDSSolverTest::n_t, WPDSSolverTest::d_t,
//       WPDSSolverTest::f_t,
//                  WPDSSolverTest::t_t, WPDSSolverTest::v_t,
//                  WPDSSolverTest::l_t, WPDSSolverTest::i_t>
//           S(T);
//       S.solve();
//       auto Results = S.resultsAt(Ret);
//       cout << "Results:\n";
//       for (auto &Result : Results) {
//         Result.first->print(llvm::outs());
//         cout << '\n';
//       }
//     } else if (DFA == "LCA") {
//       std::cout << "LCA" << std::endl;
//       // WPDSLinearConstantAnalysis L(&DB, &H, &I, &PT, {"main"});
//       // WPDSSolver<
//       //     WPDSLinearConstantAnalysis::n_t,
//       WPDSLinearConstantAnalysis::d_t,
//       //     WPDSLinearConstantAnalysis::f_t,
//       WPDSLinearConstantAnalysis::t_t,
//       //     WPDSLinearConstantAnalysis::v_t,
//       WPDSLinearConstantAnalysis::l_t,
//       //     WPDSLinearConstantAnalysis::i_t>
//       //     S(L);
//       // S.solve();
//       // auto Results = S.resultsAt(Ret);
//       // cout << "Results:\n";
//       // if (!Results.empty()) {
//       //   for (auto &Result : Results) {
//       //     Result.first->print(llvm::outs());
//       //     cout << " - with value: ";
//       //     L.printEdgeFact(cout, Result.second);
//       //     std::cout << '\n';
//       //   }
//       // } else {
//       //   cout << "Results are empty!\n";
//       // }
//     }
//     std::cout << "DONE!\n";
//   } else {
//     std::cerr << "error: file does not contain a 'main' function!\n";
//   }
//   return 0;
// }

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
