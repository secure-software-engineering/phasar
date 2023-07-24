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
  GroundTruth.emplace("$s9branch_066MyMainV7wrapperyS2iFZ", 13, "x", 10);
  GroundTruth.emplace("$s9branch_066MyMainV7wrapperyS2iFZ", 13, "i", 10);
  GroundTruth.emplace("$s9branch_066MyMainV7wrapperyS2iFZ", 13, "j", 10);
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

/* ============== CALL TESTS ============== */
TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_01) {
  auto Results = doAnalysis("call_01.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s7call_016MyMainV3fooyySiFZ", 4, "a", 42);
  GroundTruth.emplace("$s7call_016MyMainV3fooyySiFZ", 4, "b", 42);

  GroundTruth.emplace("$s7call_016MyMainV4mainyyFZ", 8, "i", 42);
  GroundTruth.emplace("$s7call_016MyMainV4mainyyFZ", 9, "i", 42);
  compareResults(Results, GroundTruth);
  EXPECT_EQ(Results["_Z3fooi"].find(4), Results["_Z3fooi"].end());
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_02) {
  auto Results = doAnalysis("call_02.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s7call_026MyMainV3fooyS2iFZ", 4, "a", 2);
  GroundTruth.emplace("$s7call_026MyMainV4mainyyFZ", 9, "i", 42);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["main"].find(6) == Results["main"].end());
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_03) {
  auto Results = doAnalysis("call_03.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s7call_036MyMainV4mainyyFZ", 8, "i", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_04) {
  auto Results = doAnalysis("call_04.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s7call_046MyMainV4mainyyFZ", 8, "i", 10);
  GroundTruth.emplace("$s7call_046MyMainV4mainyyFZ", 9, "i", 10);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_05) {
  auto Results = doAnalysis("call_05.ll");
  std::set<LCACompactResult_t> GroundTruth;
  EXPECT_TRUE(Results["main"].empty());
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_06) {
  auto Results = doAnalysis("call_06.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s7call_066MyMainV9incrementyS2iFZ", 4, "a", 1);

  GroundTruth.emplace("$s7call_066MyMainV4mainyyFZ", 8, "i", 42);
  GroundTruth.emplace("$s7call_066MyMainV4mainyyFZ", 9, "i", 2);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_07) {
  auto Results = doAnalysis("call_07.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s7call_076MyMainV4mainyyFZ", 8, "i", 42);
  GroundTruth.emplace("$s7call_076MyMainV4mainyyFZ", 9, "i", 42);
  GroundTruth.emplace("$s7call_076MyMainV4mainyyFZ", 9, "j", 43);
  GroundTruth.emplace("$s7call_076MyMainV4mainyyFZ", 10, "i", 42);
  GroundTruth.emplace("$s7call_076MyMainV4mainyyFZ", 10, "j", 43);
  GroundTruth.emplace("$s7call_076MyMainV4mainyyFZ", 10, "k", 44);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["$s7call_076MyMainV9incrementyS2iFZ"].find(1) ==
              Results["$s7call_076MyMainV9incrementyS2iFZ"].end());
  EXPECT_TRUE(Results["$s7call_076MyMainV9incrementyS2iFZ"].find(2) ==
              Results["$s7call_076MyMainV9incrementyS2iFZ"].end());
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_08) {
  auto Results = doAnalysis("call_08.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s7call_086MyMainV3fooyS2i_SitFZ", 4, "a", 10);
  GroundTruth.emplace("$s7call_086MyMainV3fooyS2i_SitFZ", 4, "b", 1);

  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 8, "i", 10);
  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 9, "i", 10);
  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 9, "j", 1);
  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 11, "i", 10);
  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 11, "j", 1);

  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_09) {
  auto Results = doAnalysis("call_09.ll");
  std::set<LCACompactResult_t> GroundTruth;

  GroundTruth.emplace("$s7call_096MyMainV4mainyyFZ", 8, "i", 43);
  GroundTruth.emplace("$s7call_096MyMainV4mainyyFZ", 9, "i", 43);
  GroundTruth.emplace("$s7call_096MyMainV4mainyyFZ", 9, "j", 43);

  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_10) {
  auto Results = doAnalysis("call_10.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s7call_106MyMainV3baryySiFZ", 3, "b", 2);
  GroundTruth.emplace("$s7call_106MyMainV3fooyySiFZ", 8, "a", 2);

  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_11) {
  auto Results = doAnalysis("call_11.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s7call_116MyMainV3baryS2iFZ", 4, "b", 2);

  GroundTruth.emplace("$s7call_116MyMainV3fooyS2iFZ", 8, "a", 2);

  GroundTruth.emplace("$s7call_116MyMainV4mainyyFZ", 13, "i", 2);
  compareResults(Results, GroundTruth);
}

/* ============== LOOP TESTS ============== */
TEST_F(IDELinearConstantAnalysisSwiftTest, HandleLoopTest_01) {
  auto Results = doAnalysis("while_01.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8while_016MyMainV4mainyyFZ", 4, "i", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleLoopTest_03) {
  auto Results = doAnalysis("while_03.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8while_036MyMainV4mainyyFZ", 4, "i", 42);
  GroundTruth.emplace("$s8while_036MyMainV4mainyyFZ", 9, "a", 13);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleLoopTest_04) {
  auto Results = doAnalysis("while_04.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8while_046MyMainV4mainyyFZ", 4, "i", 42);
  GroundTruth.emplace("$s8while_046MyMainV4mainyyFZ", 7, "a", 0);
  compareResults(Results, GroundTruth);
}

/* ============== Global TESTS ============== */

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_01) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_01.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9global_016MyMainV4mainyyFZ", 8, "i", 666);
  GroundTruth.emplace("$s9global_016MyMainV4mainyyFZ", 8, "g1", 10);
  GroundTruth.emplace("$s9global_016MyMainV4mainyyFZ", 8, "g2", 1);
  GroundTruth.emplace("$s9global_016MyMainV4mainyyFZ", 10, "i", 666);
  GroundTruth.emplace("$s9global_016MyMainV4mainyyFZ", 10, "g1", 42);
  GroundTruth.emplace("$s9global_016MyMainV4mainyyFZ", 10, "g2", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_02) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_02.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9global_026MyMainV4mainyyFZ", 6, "g", 10);
  GroundTruth.emplace("$s9global_026MyMainV4mainyyFZ", 6, "i", 10);
  GroundTruth.emplace("$s9global_026MyMainV4mainyyFZ", 7, "g", 10);
  GroundTruth.emplace("$s9global_026MyMainV4mainyyFZ", 7, "i", -10);
  GroundTruth.emplace("$s9global_026MyMainV4mainyyFZ", 8, "g", -10);
  GroundTruth.emplace("$s9global_026MyMainV4mainyyFZ", 8, "i", -10);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_03) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_03.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9global_036MyMainV3fooyyFZ", 10, "g", 0);
  GroundTruth.emplace("$s9global_036MyMainV4mainyyFZ", 10, "i", 42);
  GroundTruth.emplace("$s9global_036MyMainV4mainyyFZ", 11, "i", 42);
  GroundTruth.emplace("$s9global_036MyMainV4mainyyFZ", 11, "g", 1);
  GroundTruth.emplace("$s9global_036MyMainV4mainyyFZ", 12, "i", 42);
  GroundTruth.emplace("$s9global_036MyMainV4mainyyFZ", 12, "g", 2);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_04) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_04.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9global_046MyMainV3fooyS2iFZ", 7, "g", 1);
  GroundTruth.emplace("$s9global_046MyMainV3fooyS2iFZ", 7, "a", 2);

  GroundTruth.emplace("$s9global_046MyMainV4mainyyFZ", 12, "g", 1);
  GroundTruth.emplace("$s9global_046MyMainV4mainyyFZ", 12, "i", 2);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_06) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_06.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9global_066MyMainV3fooSiyFZ", 7, "g", 2);
  GroundTruth.emplace("$s9global_066MyMainV4mainyyFZ", 11, "g", 1);
  GroundTruth.emplace("$s9global_066MyMainV4mainyyFZ", 12, "g", 2);
  GroundTruth.emplace("$s9global_066MyMainV4mainyyFZ", 12, "i", 2);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_07) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_07.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9global_076MyMainV3fooyS2iFZ", 7, "g", 1);
  GroundTruth.emplace("$s9global_076MyMainV3fooyS2iFZ", 7, "a", 10);
  GroundTruth.emplace("$s9global_076MyMainV3fooyS2iFZ", 7, "x", 10);
  GroundTruth.emplace("$s9global_076MyMainV3fooyS2iFZ", 7, "g", 1);
  GroundTruth.emplace("$s9global_076MyMainV3fooyS2iFZ", 7, "a", 10);
  GroundTruth.emplace("$s9global_076MyMainV3fooyS2iFZ", 7, "x", 11);

  GroundTruth.emplace("$s9global_076MyMainV3baryS2iFZ", 14, "g", 2);
  GroundTruth.emplace("$s9global_076MyMainV3baryS2iFZ", 14, "b", 3);

  GroundTruth.emplace("$s9global_076MyMainV4mainyyFZ", 19, "g", 1);
  GroundTruth.emplace("$s9global_076MyMainV4mainyyFZ", 19, "i", 0);
  GroundTruth.emplace("$s9global_076MyMainV4mainyyFZ", 20, "g", 1);
  GroundTruth.emplace("$s9global_076MyMainV4mainyyFZ", 20, "i", 11);
  GroundTruth.emplace("$s9global_076MyMainV4mainyyFZ", 21, "i", 5);
  GroundTruth.emplace("$s9global_076MyMainV4mainyyFZ", 21, "g", 2);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_08) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_08.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9global_086MyMainV3baryS2iFZ", 11, "b", 2);
  GroundTruth.emplace("$s9global_086MyMainV3baryS2iFZ", 11, "g", 2);

  GroundTruth.emplace("$s9global_086MyMainV3bazyS2iFZ", 7, "g", 2);
  GroundTruth.emplace("$s9global_086MyMainV3bazyS2iFZ", 7, "c", 3);

  GroundTruth.emplace("$s9global_086MyMainV3fooyS2iFZ", 15, "g", 2);
  GroundTruth.emplace("$s9global_086MyMainV3fooyS2iFZ", 15, "a", 1);

  GroundTruth.emplace("$s9global_086MyMainV4mainyyFZ", 20, "g", 2);
  GroundTruth.emplace("$s9global_086MyMainV4mainyyFZ", 20, "i", 0);
  GroundTruth.emplace("$s9global_086MyMainV4mainyyFZ", 21, "i", 5);
  GroundTruth.emplace("$s9global_086MyMainV4mainyyFZ", 21, "g", 2);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_10) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_10.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9global_106MyMainV4mainyyFZ", 7, "g1", 42);
  GroundTruth.emplace("$s9global_106MyMainV4mainyyFZ", 7, "g2", 9001);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_11) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_11.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s9global_116MyMainV3fooyS2iFZ", 8, "x", 14);
  GroundTruth.emplace("$s9global_116MyMainV3fooyS2iFZ", 8, "a", 14);
  GroundTruth.emplace("$s9global_116MyMainV3fooyS2iFZ", 8, "g1", 42);
  GroundTruth.emplace("$s9global_116MyMainV3fooyS2iFZ", 8, "g2", 9001);
  GroundTruth.emplace("$s9global_116MyMainV3fooyS2iFZ", 10, "x", 14);
  GroundTruth.emplace("$s9global_116MyMainV3fooyS2iFZ", 10, "a", 15);
  GroundTruth.emplace("$s9global_116MyMainV3fooyS2iFZ", 10, "g1", 42);
  GroundTruth.emplace("$s9global_116MyMainV3fooyS2iFZ", 10, "g2", 9001);
  GroundTruth.emplace("$s9global_116MyMainV4mainyyFZ", 16, "a", 14);
  GroundTruth.emplace("$s9global_116MyMainV4mainyyFZ", 16, "g1", 42);
  GroundTruth.emplace("$s9global_116MyMainV4mainyyFZ", 16, "g2", 9001);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_12) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_12.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("global_ctor", 4, "g", 42);
  GroundTruth.emplace("$s9global_126MyMainV3fooyS2iFZ", 13, "x", 42);
  GroundTruth.emplace("$s9global_126MyMainV3fooyS2iFZ", 13, "g", 42);
  GroundTruth.emplace("$s9global_126MyMainV3fooyS2iFZ", 13, "a", 43);
  GroundTruth.emplace("$s9global_126MyMainV4mainyyFZ", 17, "a", 42);
  GroundTruth.emplace("$s9global_126MyMainV4mainyyFZ", 17, "g", 42);
  GroundTruth.emplace("$s9global_126MyMainV4mainyyFZ", 18, "a", 43);
  GroundTruth.emplace("$s9global_126MyMainV4mainyyFZ", 18, "g", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_13) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_13.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("global_ctor", 4, "g", 42);
  GroundTruth.emplace("global_dtor", 7, "g", 666);
  GroundTruth.emplace("$s9global_136MyMainV3fooyS2iFZ", 16, "x", 42);
  GroundTruth.emplace("$s9global_136MyMainV3fooyS2iFZ", 16, "g", 42);
  GroundTruth.emplace("$s9global_136MyMainV3fooyS2iFZ", 16, "a", 43);
  GroundTruth.emplace("$s9global_136MyMainV4mainyyFZ", 21, "a", 42);
  GroundTruth.emplace("$s9global_136MyMainV4mainyyFZ", 21, "g", 42);
  GroundTruth.emplace("$s9global_136MyMainV4mainyyFZ", 21, "b", 43);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_14) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_14.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_ZN1XC2Ev", 4, "g", 1024);
  GroundTruth.emplace("_Z3fooi", 9, "x", 1025);
  GroundTruth.emplace("_Z3fooi", 9, "g", 1024);
  GroundTruth.emplace("main", 15, "a", 1024);
  GroundTruth.emplace("main", 15, "g", 1024);
  GroundTruth.emplace("main", 17, "a", 1025);
  GroundTruth.emplace("main", 17, "g", 1024);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_15) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_15.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_ZN1XC2Ev", 5, "g1", 1024);
  GroundTruth.emplace("_ZN1XC2Ev", 5, "g2", 99);
  GroundTruth.emplace("_ZN1YC2Ev", 9, "g1", 1024);
  GroundTruth.emplace("_ZN1YC2Ev", 9, "g2", 100);
  GroundTruth.emplace("_ZN1YD2Ev", 10, "g1", 113);
  GroundTruth.emplace("_ZN1YD2Ev", 10, "g2", 100);
  GroundTruth.emplace("_Z3fooi", 15, "x", 1025);
  GroundTruth.emplace("_Z3fooi", 15, "g1", 1024);
  GroundTruth.emplace("_Z3fooi", 15, "g2", 100);
  GroundTruth.emplace("main", 22, "a", 1024);
  GroundTruth.emplace("main", 22, "g1", 1024);
  GroundTruth.emplace("main", 22, "g2", 100);
  GroundTruth.emplace("main", 25, "a", 1025);
  GroundTruth.emplace("main", 25, "b", 100);
  GroundTruth.emplace("main", 25, "g1", 1024);
  GroundTruth.emplace("main", 25, "g2", 100);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleGlobalsTest_16) {
  GTEST_SKIP() << "Swift-globals are not supported yet";
  auto Results = doAnalysis("global_16.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3fooi", 4, "x", 16);
  GroundTruth.emplace("_Z3fooi", 4, "g", 15);
  GroundTruth.emplace("_Z3fooi", 5, "x", 16);
  GroundTruth.emplace("_Z3fooi", 5, "g", 15);
  GroundTruth.emplace("main", 9, "a", 15);
  GroundTruth.emplace("main", 9, "g", 15);
  GroundTruth.emplace("main", 11, "a", 16);
  GroundTruth.emplace("main", 11, "g", 15);
  compareResults(Results, GroundTruth);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
