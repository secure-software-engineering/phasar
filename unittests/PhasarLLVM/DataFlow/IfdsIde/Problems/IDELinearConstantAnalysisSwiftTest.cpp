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
  auto Results = doAnalysis("basic_04.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 4, "i", 13);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_02) {
  auto Results = doAnalysis("basic_04.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 4, "i", 13);
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 5, "k", 17);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_03) {
  auto Results = doAnalysis("basic_04.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 5, "i", 10);
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 6, "j", 14);
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 6, "i", 14);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBasicTest_04) {
  auto Results = doAnalysis("basic_04.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 11, "i", 14);
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
  // TODO: This fails right now because we currently don't process
  // store i 64, i64* %._value ... instructions.
  // GroundTruth.emplace("$s8basic_076MyMainV7wrapperyS2iFZ", 11, "j", 3);
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
  // TODO: This fails right now because we currently don't process
  // store i 64, i64* %._value ... instructions.
  // GroundTruth.emplace("$s8basic_126MyMainV4mainyyFZ", 4, "i", 42);
  GroundTruth.emplace("$s8basic_126MyMainV3fooyS2iFZ", 8, "x", 42);
  compareResults(Results, GroundTruth);
}

/* ============== BRANCH TESTS ============== */
// TEST_F(IDELinearConstantAnalysisSwiftTest, HandleBranchTest_01) {
//   auto Results = doAnalysis("branch_01_cpp_dbg.ll");
//   std::set<LCACompactResult_t> GroundTruth;
//   GroundTruth.emplace("main", 3, "i", 10);
//   GroundTruth.emplace("main", 5, "i", 2);
//   compareResults(Results, GroundTruth);
//   // Results available for line 5 but not for line 7
//   EXPECT_FALSE(Results["main"].find(5) == Results["main"].end());
//   EXPECT_TRUE(Results["main"].find(7) == Results["main"].end());
// }

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
