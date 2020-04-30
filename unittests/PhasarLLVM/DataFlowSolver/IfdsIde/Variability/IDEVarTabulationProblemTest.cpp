/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#include <tuple>

#include "gtest/gtest.h"

#include "llvm/ADT/StringRef.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDEVarTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEVarTabulationProblemTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/variability/";
  const std::set<std::string> EntryPoints = {"main"};

  // Function - Line Nr - Variable - Z3Constraint x Value
  using LCAVarCompactResults_t =
      std::tuple<std::string, std::size_t, std::string,
                 std::set<std::pair<std::string, int64_t>>>;

  ProjectIRDB *IRDB = nullptr;

  void SetUp() override { boost::log::core::get()->set_logging_enabled(false); }

  // IDELinearConstantAnalysis::lca_restults_t
  void
  doAnalysisAndCompareResults(const std::string &llvmFilePath,
                              std::set<LCAVarCompactResults_t> &GroundTruth,
                              bool printDump = false) {
    IRDB = new ProjectIRDB({pathToLLFiles + llvmFilePath}, IRDBOptions::WPA);
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToInfo PT(*IRDB);
    LLVMBasedVarICFG VICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, &TH,
                           &PT);
    IDELinearConstantAnalysis LCAProblem(IRDB, &TH, &VICFG, &PT, EntryPoints);
    IDEVariabilityTabulationProblem_P<IDELinearConstantAnalysis> VARAProblem(
        LCAProblem, VICFG);
    IDESolver_P<IDEVariabilityTabulationProblem_P<IDELinearConstantAnalysis>>
        LCASolver(VARAProblem);
    LCASolver.solve();
    if (printDump) {
      LCASolver.dumpResults();
    }
    for (auto &Truth : GroundTruth) {
      auto Fun = IRDB->getFunctionDefinition(std::get<0>(Truth));
      auto Inst = getNthInstruction(Fun, std::get<1>(Truth));
      auto Results = LCASolver.resultsAt(Inst);
      for (auto &[Fact, Value] : Results) {
        if (llvm::StringRef(llvmIRToString(Fact))
                .startswith(std::get<2>(Truth))) {
          for (auto &[Constraint, IntegerValue] : Value) {
            bool Found = false;
            for (auto &[TrueConstaint, TrueIntegerValue] : std::get<3>(Truth)) {
              // std::cout << "Comparing: " << Constraint.to_string() << " and "
              // << TrueConstaint << '\n';
              if (Constraint.to_string() == TrueConstaint) {
                EXPECT_TRUE(IntegerValue == TrueIntegerValue);
                Found = true;
                break;
              }
            }
            if (!Found) {
              FAIL() << "Could not find constraint: '" << Constraint.to_string()
                     << "' in ground truth!";
            }
          }
        }
      }
    }
  }

  void TearDown() override { delete IRDB; }

}; // Test Fixture

// TEST_F(IDEVarTabulationProblemTest,
// HandleBasic_TwoVariablesDesugared) {
//   auto Results = doAnalysis("twovariables_desugared_c.ll", true);
//   // std::set<LCAVarCompactResults_t
// > GroundTruth;
//   // GroundTruth.emplace("main", 2, "i", 13);
//   // GroundTruth.emplace("main", 3, "i", 13);
//   // compareResults(Results, GroundTruth);
// }

TEST_F(IDEVarTabulationProblemTest, HandleBasic_01) {
  std::set<LCAVarCompactResults_t> GroundTruth;
  GroundTruth.emplace("main", 11, "x",
                      std::set<std::pair<std::string, int64_t>>{
                          {"A_defined", 42}, {"(not A_defined)", 13}});
  GroundTruth.emplace("main", 11, "retval",
                      std::set<std::pair<std::string, int64_t>>{{"true", 0}});
  doAnalysisAndCompareResults("basic_01_c.ll", GroundTruth, false);
}

TEST_F(IDEVarTabulationProblemTest, HandleBasic_02) {
  std::set<LCAVarCompactResults_t> GroundTruth;
  GroundTruth.emplace("main", 6, "x",
                      std::set<std::pair<std::string, int64_t>>{{"true", 150}});
  GroundTruth.emplace("main", 6, "%0",
                      std::set<std::pair<std::string, int64_t>>{{"true", 150}});
  GroundTruth.emplace("main", 6, "retval",
                      std::set<std::pair<std::string, int64_t>>{{"true", 0}});
  doAnalysisAndCompareResults("basic_02_c.ll", GroundTruth, false);
}

// TEST_F(IDEVarTabulationProblemTest, HandleBasic_03) {
//   // auto Results = doAnalysis("basic_01_c.ll", true);
//   doAnalysis("basic_03_c.ll", true);
//   // std::set<LCAVarCompactResults_t
// > GroundTruth;
//   // GroundTruth.emplace("main", 2, "i", 13);
//   // GroundTruth.emplace("main", 3, "i", 13);
//   // compareResults(Results, GroundTruth);
// }

// TEST_F(IDEVarTabulationProblemTest, HandleBasic_04) {
//   // auto Results = doAnalysis("basic_01_c.ll", true);
//   doAnalysis("basic_04_c.ll", true);
//   // std::set<LCAVarCompactResults_t
// > GroundTruth;
//   // GroundTruth.emplace("main", 2, "i", 13);
//   // GroundTruth.emplace("main", 3, "i", 13);
//   // compareResults(Results, GroundTruth);
// }

TEST_F(IDEVarTabulationProblemTest, HandleLoops_01) {
  std::set<LCAVarCompactResults_t> GroundTruth;
  // GroundTruth.emplace("main", 6, "x",
  // std::set<std::pair<std::string, int64_t>>{{"true", 150}});
  // FIXME: loops do not work, yet. It's probably better if we model
  // the constraints (z3::expr) as edge functions as well.
  doAnalysisAndCompareResults("loop_01_c.ll", GroundTruth, true);
}

TEST_F(IDEVarTabulationProblemTest, HandleCalls_01) {
  std::set<LCAVarCompactResults_t> GroundTruth;
  doAnalysisAndCompareResults("call_01_c.ll", GroundTruth, false);
}

// TEST_F(IDEVarTabulationProblemTest, HandleRecursion_01) {
//   std::set<LCAVarCompactResults_t> GroundTruth;
// // FIXME: recursion does not work, yet. It's probably better if we model
// // the constraints (z3::expr) as edge functions as well.
//   doAnalysisAndCompareResults("recursion_01_c.ll", GroundTruth, true);
// }

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
