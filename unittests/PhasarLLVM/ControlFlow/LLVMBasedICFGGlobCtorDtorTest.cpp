/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "gtest/gtest.h"

#include <array>
#include <string>
#include <vector>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

static const std::string PathToLLFiles =
    unittest::PathToLLTestFiles + "globals/";

static void EnsureFunctionOrdering(
    LLVMBasedICFG &ICFG,
    llvm::SmallDenseSet<const llvm::Function *, 4> Functions,
    std::vector<std::array<const llvm::Function *, 2>> FixedOrdering = {}) {
  if (Functions.size() < 2)
    return;

  llvm::SmallDenseMap<const llvm::Function *, const llvm::Function *, 8> next,
      prev;

  // Construct ordering between the functions...
  for (auto *Fun : Functions) {
    auto succ = ICFG.getSuccsOf(&Fun->back().back());
    auto pred = ICFG.getPredsOf(&Fun->front().front());

    EXPECT_TRUE(succ.size() < 2);
    EXPECT_TRUE(pred.size() < 2);

    if (succ.empty()) {
      ASSERT_EQ(1, pred.size());
    } else if (pred.empty()) {
      ASSERT_EQ(1, succ.size());
    }

    auto succFn = succ.empty() ? nullptr : succ.front()->getFunction();
    auto predFn = pred.empty() ? nullptr : pred.front()->getFunction();

    if (Fun != succFn)
      EXPECT_TRUE(next.insert({Fun, succFn}).second);
    if (Fun != predFn)
      EXPECT_TRUE(prev.insert({Fun, predFn}).second);

    llvm::outs() << (predFn ? predFn->getName() : "null") << " < "
                 << Fun->getName() << " < "
                 << (succFn ? succFn->getName() : "null") << "\n";

    if (!predFn) {
      EXPECT_TRUE(next.insert({nullptr, Fun}).second);
    } else if (!succFn) {
      EXPECT_TRUE(prev.insert({nullptr, Fun}).second);
    }
  }

  // Check that the ordering is indeed total

  llvm::SmallDenseSet<const llvm::Function *, 8> visited;
  llvm::SmallDenseMap<const llvm::Function *, size_t, 8> FunIdx;

  auto Curr = next[nullptr];
  bool finished = false;
  size_t idx = 0;

  do {
    FunIdx[Curr] = idx++;

    finished = !visited.insert(Curr).second;
    Curr = next[Curr];
  } while (Curr && !finished);

  EXPECT_EQ(Functions.size(), visited.size());

  visited.clear();

  Curr = prev[nullptr];
  do {
    finished = !visited.insert(Curr).second;
    Curr = prev[Curr];
  } while (Curr && !finished);

  EXPECT_EQ(Functions.size(), visited.size());

  // Ensure the fixed ordering

  for (auto &[From, To] : FixedOrdering) {
    EXPECT_LT(FunIdx[From], FunIdx[To]);
  }
}

TEST(LLVMBasedICFGGlobCtorDtorTest, CtorTest) {
  boost::log::core::get()->set_logging_enabled(false);
  ValueAnnotationPass::resetValueID();

  ProjectIRDB IRDB({PathToLLFiles + "globals_ctor_1_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::SOUNDY, /*IncludeGlobals*/ true);

  auto *GlobalCtor = ICFG.getFirstGlobalCtorOrNull();
  EXPECT_TRUE(GlobalCtor != nullptr);

  auto *ExpectedGlobalCtor =
      IRDB.getFunction("_GLOBAL__sub_I_globals_ctor_1.cpp");

  EXPECT_EQ(ExpectedGlobalCtor, GlobalCtor);

  auto *MainFn = IRDB.getFunction("main");

  EnsureFunctionOrdering(ICFG, {MainFn, ExpectedGlobalCtor},
                         {{ExpectedGlobalCtor, MainFn}});
}

TEST(LLVMBasedICFGGlobCtorDtorTest, CtorTest2) {
  boost::log::core::get()->set_logging_enabled(false);
  ValueAnnotationPass::resetValueID();

  ProjectIRDB IRDB({PathToLLFiles + "globals_ctor_2_1_cpp.ll",
                    PathToLLFiles + "globals_ctor_2_2_cpp.ll"},
                   IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::SOUNDY, /*IncludeGlobals*/ true);

  auto *GlobalCtor = ICFG.getFirstGlobalCtorOrNull();
  EXPECT_TRUE(GlobalCtor != nullptr);

  // IRDB.print();

  auto *FirstGlobCtor = IRDB.getFunction("_GLOBAL__sub_I_globals_ctor_2_1.cpp");
  auto *SecondGlobCtor =
      IRDB.getFunction("_GLOBAL__sub_I_globals_ctor_2_2.cpp");

  auto *MainFn = IRDB.getFunction("main");

  EnsureFunctionOrdering(ICFG, {FirstGlobCtor, SecondGlobCtor, MainFn},
                         {{FirstGlobCtor, MainFn}, {SecondGlobCtor, MainFn}});
}

TEST(LLVMBasedICFGGlobCtorDtorTest, DtorTest1) {
  boost::log::core::get()->set_logging_enabled(false);
  ValueAnnotationPass::resetValueID();

  ProjectIRDB IRDB({PathToLLFiles + "globals_dtor_1_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::SOUNDY, /*IncludeGlobals*/ true);

  auto *GlobalDtor = IRDB.getFunction("_ZN3FooD2Ev");
  auto *MainFn = IRDB.getFunction("main");
  auto *GlobalDtorInit = IRDB.getFunction("_GLOBAL__sub_I_globals_dtor_1.cpp");

  EnsureFunctionOrdering(ICFG, {MainFn, GlobalDtor, GlobalDtorInit},
                         {{MainFn, GlobalDtor}, {GlobalDtorInit, MainFn}});
}

TEST(LLVMBasedICFGGlobCtorDtorTest, DISABLED_LCATest1) {
  boost::log::core::get()->set_logging_enabled(false);
  ValueAnnotationPass::resetValueID();

  ProjectIRDB IRDB({PathToLLFiles + "globals_lca_1_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::SOUNDY, /*IncludeGlobals*/ true);
  IDELinearConstantAnalysis Problem(
      &IRDB, &TH, &ICFG, &PT, {"main", "_GLOBAL__sub_I_globals_lca_1.cpp"});

  IDESolver_P<IDELinearConstantAnalysis> Solver(Problem);

  Solver.solve();

  auto *LoadX = IRDB.getInstruction(11);
  auto *PrintF = IRDB.getInstruction(12);

  auto XValueAtPrintF = Solver.resultAt(PrintF, LoadX);

  // Currently computes 1
  EXPECT_EQ(43, XValueAtPrintF);
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
