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
  GroundTruth.emplace("$s7call_086MyMainV3fooyS2i_SitFZ", 3, "a", 10);
  GroundTruth.emplace("$s7call_086MyMainV3fooyS2i_SitFZ", 3, "b", 1);

  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 8, "i", 10);
  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 9, "i", 10);
  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 9, "j", 1);
  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 10, "i", 10);
  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 10, "j", 1);
  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 11, "k", 11);
  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 11, "i", 10);
  GroundTruth.emplace("$s7call_086MyMainV4mainyyFZ", 11, "j", 1);

  compareResults(Results, GroundTruth);
}

// TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_09) {
//   auto Results = doAnalysis("call_09.ll");
//   std::set<LCACompactResult_t> GroundTruth;
//   GroundTruth.emplace("_Z9incrementi", 1, "a", 42);
//   GroundTruth.emplace("_Z9incrementi", 2, "a", 43);

//   GroundTruth.emplace("main", 6, "i", 43);
//   GroundTruth.emplace("main", 7, "i", 43);
//   GroundTruth.emplace("main", 7, "j", 43);
//   GroundTruth.emplace("main", 8, "i", 43);
//   GroundTruth.emplace("main", 8, "j", 43);
//   compareResults(Results, GroundTruth);
// }

// TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_10) {
//   auto Results = doAnalysis("call_10.ll");
//   std::set<LCACompactResult_t> GroundTruth;
//   GroundTruth.emplace("_Z3bari", 1, "b", 2);
//   GroundTruth.emplace("_Z3fooi", 3, "a", 2);
//   GroundTruth.emplace("_Z3fooi", 4, "a", 2);
//   compareResults(Results, GroundTruth);
//   EXPECT_TRUE(Results["main"].find(8) == Results["main"].end());
//   EXPECT_TRUE(Results["main"].find(9) == Results["main"].end());
// }

// TEST_F(IDELinearConstantAnalysisSwiftTest, HandleCallTest_11) {
//   auto Results = doAnalysis("call_11.ll");
//   std::set<LCACompactResult_t> GroundTruth;
//   GroundTruth.emplace("_Z3bari", 1, "b", 2);
//   GroundTruth.emplace("_Z3bari", 2, "b", 2);

//   GroundTruth.emplace("_Z3fooi", 5, "a", 2);
//   GroundTruth.emplace("_Z3fooi", 6, "a", 2);

//   GroundTruth.emplace("main", 11, "i", 2);
//   GroundTruth.emplace("main", 12, "i", 2);
//   compareResults(Results, GroundTruth);
// }

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
