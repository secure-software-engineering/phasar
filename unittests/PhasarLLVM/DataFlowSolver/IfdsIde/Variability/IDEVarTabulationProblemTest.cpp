/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#include <tuple>

#include <gtest/gtest.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVarICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDEVarTabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEVarTabulationProblemTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/variability/";
  const std::set<std::string> EntryPoints = {"main"};

  // Function - Line Nr - Variable - Value
  using LCACompactResult_t =
      std::tuple<std::string, std::size_t, std::string, int64_t>;
  ProjectIRDB *IRDB = nullptr;

  void SetUp() override { boost::log::core::get()->set_logging_enabled(false); }

  // IDELinearConstantAnalysis::lca_restults_t
  void doAnalysis(const std::string &llvmFilePath, bool printDump = false) {
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
    // return LCAProblem.getLCAResults(LCASolver.getSolverResults());
  }

  void TearDown() override { delete IRDB; }

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  void compareResults(IDELinearConstantAnalysis::lca_results_t &Results,
                      std::set<LCACompactResult_t> &GroundTruth) {
    std::set<LCACompactResult_t> RelevantResults;
    for (auto G : GroundTruth) {
      std::string fName = std::get<0>(G);
      unsigned line = std::get<1>(G);
      if (Results.find(fName) != Results.end()) {
        if (auto it = Results[fName].find(line); it != Results[fName].end()) {
          for (auto varToVal : it->second.variableToValue) {
            RelevantResults.emplace(fName, line, varToVal.first,
                                    varToVal.second);
          }
        }
      }
    }
    EXPECT_EQ(RelevantResults, GroundTruth);
  }
}; // Test Fixture

// TEST_F(IDEVarTabulationProblemTest,
// HandleBasic_TwoVariablesDesugared) {
//   auto Results = doAnalysis("twovariables_desugared_c.ll", true);
//   // std::set<LCACompactResult_t> GroundTruth;
//   // GroundTruth.emplace("main", 2, "i", 13);
//   // GroundTruth.emplace("main", 3, "i", 13);
//   // compareResults(Results, GroundTruth);
// }

TEST_F(IDEVarTabulationProblemTest, HandleBasic_01) {
  // auto Results = doAnalysis("basic_01_c.ll", true);
  doAnalysis("basic_01_c.ll", true);
  // std::set<LCACompactResult_t> GroundTruth;
  // GroundTruth.emplace("main", 2, "i", 13);
  // GroundTruth.emplace("main", 3, "i", 13);
  // compareResults(Results, GroundTruth);
}

// TEST_F(IDEVarTabulationProblemTest, HandleBasic_02) {
//   // auto Results = doAnalysis("basic_01_c.ll", true);
//   doAnalysis("basic_02_c.ll", false);
//   // std::set<LCACompactResult_t> GroundTruth;
//   // GroundTruth.emplace("main", 2, "i", 13);
//   // GroundTruth.emplace("main", 3, "i", 13);
//   // compareResults(Results, GroundTruth);
// }

// TEST_F(IDEVarTabulationProblemTest, HandleBasic_03) {
//   // auto Results = doAnalysis("basic_01_c.ll", true);
//   doAnalysis("basic_03_c.ll", true);
//   // std::set<LCACompactResult_t> GroundTruth;
//   // GroundTruth.emplace("main", 2, "i", 13);
//   // GroundTruth.emplace("main", 3, "i", 13);
//   // compareResults(Results, GroundTruth);
// }

// TEST_F(IDEVarTabulationProblemTest, HandleBasic_04) {
//   // auto Results = doAnalysis("basic_01_c.ll", true);
//   doAnalysis("basic_04_c.ll", true);
//   // std::set<LCACompactResult_t> GroundTruth;
//   // GroundTruth.emplace("main", 2, "i", 13);
//   // GroundTruth.emplace("main", 3, "i", 13);
//   // compareResults(Results, GroundTruth);
// }

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
