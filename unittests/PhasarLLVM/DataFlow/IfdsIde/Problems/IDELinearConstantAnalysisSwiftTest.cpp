#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <memory>
#include <tuple>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDELinearConstantAnalysisSwiftTest : public ::testing::Test {
protected:
  static constexpr auto PathToSwiftTestFiles = PHASAR_BUILD_SWIFT_SUBFOLDER(
      "linear_constant/"); // Function - Line Nr - Variable - Value
  const std::vector<std::string> EntryPoints = {"main"};

  // Function - Line Nr - Variable - Value
  using LCACompactResult_t = std::tuple<std::string, std::size_t, std::string,
                                        IDELinearConstantAnalysisDomain::l_t>;

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  IDELinearConstantAnalysis::lca_results_t
  doAnalysis(llvm::StringRef LlvmFilePath, bool PrintDump = false) {
    HelperAnalyses HA(PathToSwiftTestFiles + LlvmFilePath, EntryPoints);

    // Compute the ICFG to possibly create the runtime model
    auto &ICFG = HA.getICFG();

    auto HasGlobalCtor = HA.getProjectIRDB().getFunctionDefinition(
                             LLVMBasedICFG::GlobalCRuntimeModelName) != nullptr;

    auto LCAProblem = createAnalysisProblem<IDELinearConstantAnalysis>(
        HA,
        std::vector{HasGlobalCtor ? LLVMBasedICFG::GlobalCRuntimeModelName.str()
                                  : "main"});
    IDESolver LCASolver(LCAProblem, &ICFG);
    LCASolver.solve();
    if (PrintDump) {
      HA.getProjectIRDB().dump();
      ICFG.print();
      LCASolver.dumpResults();
    }
    return LCAProblem.getLCAResults(LCASolver.getSolverResults());
  }

  void TearDown() override {}

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  static void compareResults(IDELinearConstantAnalysis::lca_results_t &Results,
                             std::set<LCACompactResult_t> &GroundTruth) {
    std::set<LCACompactResult_t> RelevantResults;
    for (auto G : GroundTruth) {
      std::string FName = std::get<0>(G);
      unsigned Line = std::get<1>(G);
      if (Results.find(FName) != Results.end()) {
        if (auto It = Results[FName].find(Line); It != Results[FName].end()) {
          for (const auto &VarToVal : It->second.VariableToValue) {
            RelevantResults.emplace(FName, Line, VarToVal.first,
                                    VarToVal.second);
          }
        }
      }
    }
    EXPECT_EQ(RelevantResults, GroundTruth);
  }
}; // Test Fixture

/* ============== BASIC TESTS ============== */
TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_01) {
  auto Results = doAnalysis("basic_01.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_016MyMainV4mainyyFZ", 4, "i", 13);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_02) {
  auto Results = doAnalysis("basic_02.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_026MyMainV4mainyyFZ", 4, "i", 13);
  GroundTruth.emplace("$s8basic_026MyMainV4mainyyFZ", 5, "i", 17);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_03) {
  auto Results = doAnalysis("basic_03.ll");
  std::set<LCACompactResult_t> GroundTruth;

  GroundTruth.emplace("$s8basic_036MyMainV4mainyyFZ", 6, "j", 14);
  GroundTruth.emplace("$s8basic_036MyMainV4mainyyFZ", 6, "i", 14);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_04) {
  auto Results = doAnalysis("basic_04.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 11, "i", 14);
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 11, "j", 20);
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 11, "k", 34);
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 11, "x", 14);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_05) {
  auto Results = doAnalysis("basic_05.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_056MyMainV7wrapperyS2iFZ", 10, "i", 3);
  GroundTruth.emplace("$s8basic_056MyMainV7wrapperyS2iFZ", 10, "j", 14);
  GroundTruth.emplace("$s8basic_056MyMainV7wrapperyS2iFZ", 10, "x", 3);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_06) {
  auto Results = doAnalysis("basic_06.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_066MyMainV7wrapperyS2iFZ", 10, "i", 16);
  GroundTruth.emplace("$s8basic_066MyMainV7wrapperyS2iFZ", 10, "x", 4);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_07) {
  auto Results = doAnalysis("basic_07.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_076MyMainV7wrapperyS2iFZ", 11, "i", 16);
  GroundTruth.emplace("$s8basic_076MyMainV7wrapperyS2iFZ", 11, "x", 4);
  GroundTruth.emplace("$s8basic_076MyMainV7wrapperyS2iFZ", 11, "j", 3);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_08) {
  auto Results = doAnalysis("basic_08.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_086MyMainV7wrapperyS2iFZ", 10, "i", 42);
  GroundTruth.emplace("$s8basic_086MyMainV7wrapperyS2iFZ", 10, "j", 40);
  GroundTruth.emplace("$s8basic_086MyMainV7wrapperyS2iFZ", 10, "x", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_09) {
  auto Results = doAnalysis("basic_09.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_096MyMainV7wrapperyS2iFZ", 10, "i", 42);
  GroundTruth.emplace("$s8basic_096MyMainV7wrapperyS2iFZ", 10, "j", 126);
  GroundTruth.emplace("$s8basic_096MyMainV7wrapperyS2iFZ", 10, "x", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_10) {
  auto Results = doAnalysis("basic_10.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_106MyMainV7wrapperyS2iFZ", 10, "i", 42);
  GroundTruth.emplace("$s8basic_106MyMainV7wrapperyS2iFZ", 10, "j", 14);
  GroundTruth.emplace("$s8basic_106MyMainV7wrapperyS2iFZ", 10, "x", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_11) {
  auto Results = doAnalysis("basic_11.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_116MyMainV7wrapperyS2iFZ", 10, "i", 42);
  GroundTruth.emplace("$s8basic_116MyMainV7wrapperyS2iFZ", 10, "j", 2);
  GroundTruth.emplace("$s8basic_116MyMainV7wrapperyS2iFZ", 10, "x", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_12) {
  auto Results = doAnalysis("basic_12.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_126MyMainV4mainyyFZ", 5, "i", 43);
  GroundTruth.emplace("$s8basic_126MyMainV4mainyyFZ", 4, "i", 42);
  GroundTruth.emplace("$s8basic_126MyMainV3fooyS2iFZ", 8, "x", 42);
  compareResults(Results, GroundTruth);
}

/* ============== BRANCH TESTS ============== */
TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBranchTest_01) {
  auto Results = doAnalysis("branch_01.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9branch_016MyMainV7wrapperyS2iFZ", 10, "i", 10);
  GroundTruth.emplace("$s9branch_016MyMainV7wrapperyS2iFZ", 10, "x", 10);
  GroundTruth.emplace("$s9branch_016MyMainV7wrapperyS2iFZ", 12, "i", 2);
  GroundTruth.emplace("$s9branch_016MyMainV7wrapperyS2iFZ", 12, "x", 10);

  compareResults(Results, GroundTruth);
  EXPECT_FALSE(Results["$s9branch_016MyMainV7wrapperyS2iFZ"].find(12) ==
               Results["$s9branch_016MyMainV7wrapperyS2iFZ"].end());
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBranchTest_02) {
  auto Results = doAnalysis("branch_02.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9branch_026MyMainV7wrapperyS2iFZ", 10, "x", 10);
  GroundTruth.emplace("$s9branch_026MyMainV7wrapperyS2iFZ", 12, "i", 10);
  GroundTruth.emplace("$s9branch_026MyMainV7wrapperyS2iFZ", 12, "x", 10);
  GroundTruth.emplace("$s9branch_026MyMainV7wrapperyS2iFZ", 14, "x", 10);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBranchTest_03) {
  auto Results = doAnalysis("branch_03.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9branch_036MyMainV7wrapperyS2iFZ", 10, "x", 10);
  GroundTruth.emplace("$s9branch_036MyMainV7wrapperyS2iFZ", 10, "i", 42);
  GroundTruth.emplace("$s9branch_036MyMainV7wrapperyS2iFZ", 12, "x", 10);
  GroundTruth.emplace("$s9branch_036MyMainV7wrapperyS2iFZ", 12, "i", 10);
  GroundTruth.emplace("$s9branch_036MyMainV7wrapperyS2iFZ", 15, "x", 10);
  GroundTruth.emplace("$s9branch_036MyMainV7wrapperyS2iFZ", 15, "i", 30);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBranchTest_04) {
  auto Results = doAnalysis("branch_04.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9branch_046MyMainV7wrapperyS2iFZ", 11, "x", 10);
  GroundTruth.emplace("$s9branch_046MyMainV7wrapperyS2iFZ", 11, "i", 42);
  GroundTruth.emplace("$s9branch_046MyMainV7wrapperyS2iFZ", 11, "j", 10);
  GroundTruth.emplace("$s9branch_046MyMainV7wrapperyS2iFZ", 13, "x", 10);
  GroundTruth.emplace("$s9branch_046MyMainV7wrapperyS2iFZ", 13, "i", 20);
  GroundTruth.emplace("$s9branch_046MyMainV7wrapperyS2iFZ", 13, "j", 10);
  GroundTruth.emplace("$s9branch_046MyMainV7wrapperyS2iFZ", 15, "x", 10);
  GroundTruth.emplace("$s9branch_046MyMainV7wrapperyS2iFZ", 15, "j", 10);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBranchTest_05) {
  auto Results = doAnalysis("branch_05.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9branch_056MyMainV7wrapperyS2iFZ", 11, "x", 10);
  GroundTruth.emplace("$s9branch_056MyMainV7wrapperyS2iFZ", 11, "i", 42);
  GroundTruth.emplace("$s9branch_056MyMainV7wrapperyS2iFZ", 11, "j", 10);
  GroundTruth.emplace("$s9branch_056MyMainV7wrapperyS2iFZ", 13, "x", 10);
  GroundTruth.emplace("$s9branch_056MyMainV7wrapperyS2iFZ", 13, "i", 42);
  GroundTruth.emplace("$s9branch_056MyMainV7wrapperyS2iFZ", 13, "j", 10);
  GroundTruth.emplace("$s9branch_056MyMainV7wrapperyS2iFZ", 15, "x", 10);
  GroundTruth.emplace("$s9branch_056MyMainV7wrapperyS2iFZ", 15, "j", 10);
  GroundTruth.emplace("$s9branch_056MyMainV7wrapperyS2iFZ", 15, "i", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBranchTest_06) {
  auto Results = doAnalysis("branch_06.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9branch_066MyMainV7wrapperyS2iFZ", 11, "x", 10);
  GroundTruth.emplace("$s9branch_066MyMainV7wrapperyS2iFZ", 11, "i", 10);
  GroundTruth.emplace("$s9branch_066MyMainV7wrapperyS2iFZ", 11, "j", 10);
  GroundTruth.emplace("$s9branch_066MyMainV7wrapperyS2iFZ", 12, "x", 10);
  GroundTruth.emplace("$s9branch_066MyMainV7wrapperyS2iFZ", 12, "i", 10);
  GroundTruth.emplace("$s9branch_066MyMainV7wrapperyS2iFZ", 15, "x", 10);
  GroundTruth.emplace("$s9branch_066MyMainV7wrapperyS2iFZ", 15, "i", 10);
  GroundTruth.emplace("$s9branch_066MyMainV7wrapperyS2iFZ", 15, "j", 9);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBranchTest_07) {
  auto Results = doAnalysis("branch_07.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9branch_076MyMainV7wrapperyS2iFZ", 11, "x", 10);
  GroundTruth.emplace("$s9branch_076MyMainV7wrapperyS2iFZ", 11, "i", 30);
  GroundTruth.emplace("$s9branch_076MyMainV7wrapperyS2iFZ", 11, "j", 10);
  GroundTruth.emplace("$s9branch_076MyMainV7wrapperyS2iFZ", 13, "x", 10);
  GroundTruth.emplace("$s9branch_076MyMainV7wrapperyS2iFZ", 13, "i", 30);
  GroundTruth.emplace("$s9branch_076MyMainV7wrapperyS2iFZ", 13, "j", 10);
  GroundTruth.emplace("$s9branch_076MyMainV7wrapperyS2iFZ", 15, "x", 10);
  GroundTruth.emplace("$s9branch_076MyMainV7wrapperyS2iFZ", 15, "j", 9);
  GroundTruth.emplace("$s9branch_076MyMainV7wrapperyS2iFZ", 15, "i", 30);
  compareResults(Results, GroundTruth);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
