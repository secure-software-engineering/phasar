/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <string>
#include <tuple>

#include <gtest/gtest.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEInstInteractionAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/BitVectorSet.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEInstInteractionAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/inst_interaction/";
  const std::set<std::string> EntryPoints = {"main"};

  // Function - Line Nr - Variable - Values
  using IIACompactResult_t =
      std::tuple<std::string, std::size_t, std::string, int64_t>;
  ProjectIRDB *IRDB = nullptr;

  void SetUp() override { boost::log::core::get()->set_logging_enabled(false); }

  //   IDEInstInteractionAnalysis::lca_restults_t
  void doAnalysis(const std::string &llvmFilePath, bool printDump = false) {
    IRDB = new ProjectIRDB({pathToLLFiles + llvmFilePath}, IRDBOptions::WPA);
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToInfo PT(*IRDB);
    LLVMBasedICFG ICFG(*IRDB, CallGraphAnalysisType::CHA, EntryPoints, &TH,
                       &PT);
    IDEInstInteractionAnalysis IIAProblem(IRDB, &TH, &ICFG, &PT, EntryPoints);
    auto Generator = [](const llvm::Instruction *I, const llvm::Value *SrcNode,
                        const llvm::Value *DestNode) -> std::set<std::string> {
      if (I->hasMetadata()) {
        auto MD = I->getMetadata("dbg");
        std::string Data = "data";
        return {Data};
      }
      return {};
    };
    IIAProblem.registerEdgeFactGenerator(Generator);
    IDESolver<IDEInstInteractionAnalysis::n_t, IDEInstInteractionAnalysis::d_t,
              IDEInstInteractionAnalysis::f_t, IDEInstInteractionAnalysis::t_t,
              IDEInstInteractionAnalysis::v_t, IDEInstInteractionAnalysis::l_t,
              IDEInstInteractionAnalysis::i_t>
        IIASolver(IIAProblem);
    IIASolver.solve();
    if (printDump) {
      IIASolver.dumpResults();
    }
    // return IIAProblem.getIIAResults(IIASolver.getSolverResults());
  }

  void TearDown() override { delete IRDB; }

  //   /**
  //    * We map instruction id to value for the ground truth. ID has to be
  //    * a string since Argument ID's are not integer type (e.g. main.0 for
  //    argc).
  //    * @param groundTruth results to compare against
  //    * @param solver provides the results
  //    */
  //   void compareResults(IDEInstInteractionAnalysis::lca_restults_t &Results,
  //                       std::set<LCACompactResult_t> &GroundTruth) {
  //     std::set<LCACompactResult_t> RelevantResults;
  //     for (auto G : GroundTruth) {
  //       std::string fName = std::get<0>(G);
  //       unsigned line = std::get<1>(G);
  //       if (Results.find(fName) != Results.end()) {
  //         if (auto it = Results[fName].find(line); it !=
  //         Results[fName].end()) {
  //           for (auto varToVal : it->second.variableToValue) {
  //             RelevantResults.emplace(fName, line, varToVal.first,
  //                                     varToVal.second);
  //           }
  //         }
  //       }
  //     }
  //     EXPECT_EQ(RelevantResults, GroundTruth);
  //   }
}; // Test Fixture

/* ============== BASIC TESTS ============== */
TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_01) {
  // auto Results = doAnalysis("basic_01_cpp_dbg.ll");
  doAnalysis("basic_01_cpp_dbg.ll", true);
  // std::set<IIACompactResult_t> GroundTruth;
  // GroundTruth.emplace("main", 2, "i", {});
  // GroundTruth.emplace("main", 3, "i", {});
  // GroundTruth.emplace("main", 3, "j", {});
  //   compareResults(Results, GroundTruth);
}

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
