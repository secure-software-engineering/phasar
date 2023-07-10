#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "TestConfig.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <memory>
#include <tuple>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDELinearConstantAnalysisDotTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("linear_constant/");

  const std::vector<std::string> EntryPoints = {"main"};

  // Function - Line Nr - Variable - Value
  using LCACompactResult_t = std::tuple<std::string, std::size_t, std::string,
                                        IDELinearConstantAnalysisDomain::l_t>;

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  IDELinearConstantAnalysis::lca_results_t
  doAnalysis(llvm::StringRef LlvmFilePath, bool PrintDump = false,
             bool EmitESG = false) {
    HelperAnalyses HA(PathToLlFiles + LlvmFilePath, EntryPoints);
    auto LCAProblem =
        createAnalysisProblem<IDELinearConstantAnalysis>(HA, EntryPoints);
    IDESolver LCASolver(LCAProblem, &HA.getICFG());
    LCASolver.solve();
    if (EmitESG) {
      Logger::enable();
      const std::string PhasarRootPath = "./";
      LCASolver.emitESGAsDot(llvm::outs(), PhasarRootPath);
      Logger::disable();
    }
    if (PrintDump) {
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
TEST_F(IDELinearConstantAnalysisDotTest, HandleBasicTest_01) {
  auto Results = doAnalysis("basic_01.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 13);
  GroundTruth.emplace("main", 3, "i", 13);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBasicTest_02) {
  auto Results = doAnalysis("basic_02.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 13);
  GroundTruth.emplace("main", 3, "i", 17);
  GroundTruth.emplace("main", 4, "i", 17);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBasicTest_03) {
  auto Results = doAnalysis("basic_03.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 10);
  GroundTruth.emplace("main", 3, "i", 10);
  GroundTruth.emplace("main", 3, "j", 14);
  GroundTruth.emplace("main", 4, "i", 14);
  GroundTruth.emplace("main", 4, "j", 14);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBasicTest_04) {
  auto Results = doAnalysis("basic_04.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 3, "i", 14);
  GroundTruth.emplace("main", 4, "i", 14);
  GroundTruth.emplace("main", 4, "j", 20);
  GroundTruth.emplace("main", 5, "i", 14);
  GroundTruth.emplace("main", 5, "j", 20);
  GroundTruth.emplace("main", 6, "i", 14);
  GroundTruth.emplace("main", 6, "j", 20);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBasicTest_05) {
  auto Results = doAnalysis("basic_05.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 3);
  GroundTruth.emplace("main", 3, "i", 3);
  GroundTruth.emplace("main", 3, "j", 14);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBasicTest_06) {
  auto Results = doAnalysis("basic_06.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 4);
  GroundTruth.emplace("main", 3, "i", 16);
  GroundTruth.emplace("main", 4, "i", 16);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBasicTest_07) {
  auto Results = doAnalysis("basic_07.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 4);
  GroundTruth.emplace("main", 3, "i", 4);
  GroundTruth.emplace("main", 3, "j", 3);
  GroundTruth.emplace("main", 4, "j", 3);
  GroundTruth.emplace("main", 5, "j", 3);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBasicTest_08) {
  auto Results = doAnalysis("basic_08.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 42);
  GroundTruth.emplace("main", 3, "i", 42);
  GroundTruth.emplace("main", 3, "j", 40);
  GroundTruth.emplace("main", 4, "i", 42);
  GroundTruth.emplace("main", 4, "j", 40);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBasicTest_09) {
  auto Results = doAnalysis("basic_09.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 42);
  GroundTruth.emplace("main", 3, "i", 42);
  GroundTruth.emplace("main", 3, "j", 126);
  GroundTruth.emplace("main", 4, "i", 42);
  GroundTruth.emplace("main", 4, "j", 126);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBasicTest_10) {
  auto Results = doAnalysis("basic_10.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 42);
  GroundTruth.emplace("main", 3, "i", 42);
  GroundTruth.emplace("main", 3, "j", 14);
  GroundTruth.emplace("main", 4, "i", 42);
  GroundTruth.emplace("main", 4, "j", 14);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBasicTest_11) {
  auto Results = doAnalysis("basic_11.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 42);
  GroundTruth.emplace("main", 3, "i", 42);
  GroundTruth.emplace("main", 3, "j", 2);
  GroundTruth.emplace("main", 4, "i", 42);
  GroundTruth.emplace("main", 4, "j", 2);
  compareResults(Results, GroundTruth);
}

/* ============== BRANCH TESTS ============== */
TEST_F(IDELinearConstantAnalysisDotTest, HandleBranchTest_01) {
  auto Results = doAnalysis("branch_01.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 3, "i", 10);
  GroundTruth.emplace("main", 5, "i", 2);
  compareResults(Results, GroundTruth);
  // Results available for line 5 but not for line 7
  EXPECT_FALSE(Results["main"].find(5) == Results["main"].end());
  EXPECT_TRUE(Results["main"].find(7) == Results["main"].end());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBranchTest_02) {
  auto Results = doAnalysis("branch_02.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 6, "i", 10);
  compareResults(Results, GroundTruth);
  // Results available for line 6 but not for line 8
  EXPECT_FALSE(Results["main"].find(6) == Results["main"].end());
  EXPECT_TRUE(Results["main"].find(8) == Results["main"].end());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBranchTest_03) {
  auto Results = doAnalysis("branch_03.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 3, "i", 42);
  GroundTruth.emplace("main", 5, "i", 10);
  GroundTruth.emplace("main", 7, "i", 30);
  GroundTruth.emplace("main", 8, "i", 30);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBranchTest_04) {
  auto Results = doAnalysis("branch_04.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 3, "j", 10);
  GroundTruth.emplace("main", 4, "j", 10);
  GroundTruth.emplace("main", 4, "i", 42);
  GroundTruth.emplace("main", 6, "j", 10);
  GroundTruth.emplace("main", 6, "i", 20);
  GroundTruth.emplace("main", 8, "j", 10);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBranchTest_05) {
  auto Results = doAnalysis("branch_05.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 3, "j", 10);
  GroundTruth.emplace("main", 4, "j", 10);
  GroundTruth.emplace("main", 4, "i", 42);
  GroundTruth.emplace("main", 6, "j", 10);
  GroundTruth.emplace("main", 6, "i", 42);
  GroundTruth.emplace("main", 8, "j", 10);
  GroundTruth.emplace("main", 8, "i", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBranchTest_06) {
  auto Results = doAnalysis("branch_06.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 3, "i", 10);
  GroundTruth.emplace("main", 5, "i", 10);
  GroundTruth.emplace("main", 7, "i", 10);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleBranchTest_07) {
  auto Results = doAnalysis("branch_07.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 3, "j", 10);
  GroundTruth.emplace("main", 4, "j", 10);
  GroundTruth.emplace("main", 4, "i", 30);
  GroundTruth.emplace("main", 6, "j", 10);
  GroundTruth.emplace("main", 6, "i", 30);
  GroundTruth.emplace("main", 8, "j", 10);
  GroundTruth.emplace("main", 8, "i", 30);
  compareResults(Results, GroundTruth);
}

/* ============== LOOP TESTS ============== */
TEST_F(IDELinearConstantAnalysisDotTest, HandleLoopTest_01) {
  auto Results = doAnalysis("while_01.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 42);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["main"].find(4) == Results["main"].end());
  EXPECT_TRUE(Results["main"].find(6) == Results["main"].end());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleLoopTest_02) {
  auto Results = doAnalysis("while_02.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["main"].find(2) == Results["main"].end());
  EXPECT_TRUE(Results["main"].find(4) == Results["main"].end());
  EXPECT_TRUE(Results["main"].find(6) == Results["main"].end());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleLoopTest_03) {
  auto Results = doAnalysis("while_03.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 42);
  GroundTruth.emplace("main", 7, "a", 13);
  GroundTruth.emplace("main", 8, "a", 13);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["main"].find(4) == Results["main"].end());
  EXPECT_TRUE(Results["main"].find(6) == Results["main"].end());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleLoopTest_04) {
  auto Results = doAnalysis("while_04.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "i", 42);
  GroundTruth.emplace("main", 4, "a", 0);
  GroundTruth.emplace("main", 5, "a", 0);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["main"].find(7) == Results["main"].end());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleLoopTest_05) {
  auto Results = doAnalysis("for_01.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 2, "a", 0);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["main"].find(4) == Results["main"].end());
  EXPECT_TRUE(Results["main"].find(6) == Results["main"].end());
}

/* ============== CALL TESTS ============== */
TEST_F(IDELinearConstantAnalysisDotTest, HandleCallTest_01) {
  auto Results = doAnalysis("call_01.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3fooi", 1, "a", 42);
  GroundTruth.emplace("_Z3fooi", 2, "a", 42);
  GroundTruth.emplace("_Z3fooi", 2, "b", 42);

  GroundTruth.emplace("main", 6, "i", 42);
  GroundTruth.emplace("main", 7, "i", 42);
  GroundTruth.emplace("main", 8, "i", 42);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["_Z3fooi"].find(4) == Results["_Z3fooi"].end());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleCallTest_02) {
  auto Results = doAnalysis("call_02.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3fooi", 1, "a", 2);
  GroundTruth.emplace("_Z3fooi", 2, "a", 2);

  GroundTruth.emplace("main", 7, "i", 42);
  GroundTruth.emplace("main", 8, "i", 42);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["main"].find(6) == Results["main"].end());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleCallTest_03) {
  auto Results = doAnalysis("call_03.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 6, "i", 42);
  GroundTruth.emplace("main", 7, "i", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleCallTest_04) {
  auto Results = doAnalysis("call_04.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 6, "i", 10);
  GroundTruth.emplace("main", 7, "i", 10);
  GroundTruth.emplace("main", 8, "i", 10);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleCallTest_05) {
  auto Results = doAnalysis("call_05.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  EXPECT_TRUE(Results["main"].empty());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleCallTest_06) {
  auto Results = doAnalysis("call_06.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z9incrementi", 1, "a", 42);
  GroundTruth.emplace("_Z9incrementi", 2, "a", 43);

  GroundTruth.emplace("main", 6, "i", 42);
  GroundTruth.emplace("main", 7, "i", 43);
  GroundTruth.emplace("main", 8, "i", 43);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleCallTest_07) {
  auto Results = doAnalysis("call_07.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 6, "i", 42);
  GroundTruth.emplace("main", 7, "i", 42);
  GroundTruth.emplace("main", 7, "j", 43);
  GroundTruth.emplace("main", 8, "i", 42);
  GroundTruth.emplace("main", 8, "j", 43);
  GroundTruth.emplace("main", 8, "k", 44);
  GroundTruth.emplace("main", 9, "i", 42);
  GroundTruth.emplace("main", 9, "j", 43);
  GroundTruth.emplace("main", 9, "k", 44);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["_Z9incrementi"].find(1) ==
              Results["_Z9incrementi"].end());
  EXPECT_TRUE(Results["_Z9incrementi"].find(2) ==
              Results["_Z9incrementi"].end());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleCallTest_08) {
  auto Results = doAnalysis("call_08.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3fooii", 1, "a", 10);
  GroundTruth.emplace("_Z3fooii", 1, "b", 1);
  GroundTruth.emplace("_Z3fooii", 2, "a", 10);
  GroundTruth.emplace("_Z3fooii", 2, "b", 1);

  GroundTruth.emplace("main", 6, "i", 10);
  GroundTruth.emplace("main", 7, "i", 10);
  GroundTruth.emplace("main", 7, "j", 1);
  GroundTruth.emplace("main", 8, "i", 10);
  GroundTruth.emplace("main", 8, "j", 1);
  GroundTruth.emplace("main", 9, "i", 10);
  GroundTruth.emplace("main", 9, "j", 1);
  GroundTruth.emplace("main", 10, "i", 10);
  GroundTruth.emplace("main", 10, "j", 1);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleCallTest_09) {
  auto Results = doAnalysis("call_09.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z9incrementi", 1, "a", 42);
  GroundTruth.emplace("_Z9incrementi", 2, "a", 43);

  GroundTruth.emplace("main", 6, "i", 43);
  GroundTruth.emplace("main", 7, "i", 43);
  GroundTruth.emplace("main", 7, "j", 43);
  GroundTruth.emplace("main", 8, "i", 43);
  GroundTruth.emplace("main", 8, "j", 43);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleCallTest_10) {
  auto Results = doAnalysis("call_10.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3bari", 1, "b", 2);
  GroundTruth.emplace("_Z3fooi", 3, "a", 2);
  GroundTruth.emplace("_Z3fooi", 4, "a", 2);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["main"].find(8) == Results["main"].end());
  EXPECT_TRUE(Results["main"].find(9) == Results["main"].end());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleCallTest_11) {
  auto Results = doAnalysis("call_11.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3bari", 1, "b", 2);
  GroundTruth.emplace("_Z3bari", 2, "b", 2);

  GroundTruth.emplace("_Z3fooi", 5, "a", 2);
  GroundTruth.emplace("_Z3fooi", 6, "a", 2);

  GroundTruth.emplace("main", 11, "i", 2);
  GroundTruth.emplace("main", 12, "i", 2);
  compareResults(Results, GroundTruth);
}

/* ============== RECURSION TESTS ============== */

TEST_F(IDELinearConstantAnalysisDotTest, HandleRecursionTest_01) {
  auto Results = doAnalysis("recursion_01.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 10, "j", -1);
  GroundTruth.emplace("main", 11, "j", -1);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["_Z9decrementi"].find(2) ==
              Results["_Z9decrementi"].end());
  EXPECT_TRUE(Results["_Z9decrementi"].find(4) ==
              Results["_Z9decrementi"].end());
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleRecursionTest_02) {
  auto Results = doAnalysis("recursion_02.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleRecursionTest_03) {
  auto Results = doAnalysis("recursion_03.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 9, "a", 1);
  GroundTruth.emplace("main", 10, "a", 1);
  compareResults(Results, GroundTruth);
  EXPECT_TRUE(Results["_Z3fooj"].find(1) == Results["_Z3fooj"].end());
  EXPECT_TRUE(Results["_Z3fooj"].find(3) == Results["_Z3fooj"].end());
  EXPECT_TRUE(Results["_Z3fooj"].find(5) == Results["_Z3fooj"].end());
}

/* ============== GLOBAL VARIABLE TESTS ============== */

TEST_F(IDELinearConstantAnalysisDotTest, HandleGlobalsTest_01) {
  auto Results = doAnalysis("global_01.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 6, "g1", 10);
  GroundTruth.emplace("main", 6, "g2", 1);
  GroundTruth.emplace("main", 6, "i", 666);
  GroundTruth.emplace("main", 7, "g1", 42);
  GroundTruth.emplace("main", 7, "g2", 1);
  GroundTruth.emplace("main", 7, "i", 666);
  GroundTruth.emplace("main", 8, "g1", 42);
  GroundTruth.emplace("main", 8, "g2", 42);
  GroundTruth.emplace("main", 8, "i", 666);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleGlobalsTest_02) {
  auto Results = doAnalysis("global_02.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("main", 4, "g", 10);
  GroundTruth.emplace("main", 4, "i", 10);
  GroundTruth.emplace("main", 5, "g", 10);
  GroundTruth.emplace("main", 5, "i", -10);
  GroundTruth.emplace("main", 6, "g", -10);
  GroundTruth.emplace("main", 6, "i", -10);
  GroundTruth.emplace("main", 7, "g", -10);
  GroundTruth.emplace("main", 7, "i", -10);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleGlobalsTest_03) {
  auto Results = doAnalysis("global_03.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3foov", 4, "g", 2);
  GroundTruth.emplace("main", 8, "g", 0);
  GroundTruth.emplace("main", 8, "i", 42);
  GroundTruth.emplace("main", 9, "g", 1);
  GroundTruth.emplace("main", 9, "i", 42);
  GroundTruth.emplace("main", 10, "g", 2);
  GroundTruth.emplace("main", 10, "i", 42);
  GroundTruth.emplace("main", 11, "g", 2);
  GroundTruth.emplace("main", 11, "i", 42);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleGlobalsTest_04) {
  auto Results = doAnalysis("global_04.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3fooi", 3, "g", 1);
  GroundTruth.emplace("_Z3fooi", 3, "a", 1);
  GroundTruth.emplace("_Z3fooi", 4, "g", 1);
  GroundTruth.emplace("_Z3fooi", 4, "a", 2);

  GroundTruth.emplace("main", 8, "g", 1);
  GroundTruth.emplace("main", 9, "g", 1);
  GroundTruth.emplace("main", 9, "i", 2);
  GroundTruth.emplace("main", 10, "g", 1);
  GroundTruth.emplace("main", 10, "i", 2);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleGlobalsTest_05) {
  auto Results = doAnalysis("global_05.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3fooi", 3, "g", 2);
  GroundTruth.emplace("_Z3fooi", 3, "a", 2);
  GroundTruth.emplace("_Z3fooi", 4, "g", 2);
  GroundTruth.emplace("_Z3fooi", 4, "a", 3);

  GroundTruth.emplace("main", 8, "g", 1);
  GroundTruth.emplace("main", 9, "g", 2);
  GroundTruth.emplace("main", 9, "i", 3);
  GroundTruth.emplace("main", 10, "g", 2);
  GroundTruth.emplace("main", 10, "i", 3);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleGlobalsTest_06) {
  auto Results = doAnalysis("global_06.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3foov", 4, "g", 2);
  GroundTruth.emplace("main", 8, "g", 1);
  GroundTruth.emplace("main", 9, "g", 2);
  GroundTruth.emplace("main", 9, "i", 2);
  GroundTruth.emplace("main", 10, "g", 2);
  GroundTruth.emplace("main", 10, "i", 2);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleGlobalsTest_07) {
  auto Results = doAnalysis("global_07.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3fooi", 3, "g", 1);
  GroundTruth.emplace("_Z3fooi", 3, "a", 10);
  GroundTruth.emplace("_Z3fooi", 4, "g", 1);
  GroundTruth.emplace("_Z3fooi", 5, "g", 1);

  GroundTruth.emplace("_Z3bari", 8, "g", 1);
  GroundTruth.emplace("_Z3bari", 8, "b", 3);
  GroundTruth.emplace("_Z3bari", 9, "g", 2);
  GroundTruth.emplace("_Z3bari", 9, "b", 3);
  GroundTruth.emplace("_Z3bari", 10, "g", 2);
  GroundTruth.emplace("_Z3bari", 10, "b", 3);

  GroundTruth.emplace("main", 14, "g", 1);
  GroundTruth.emplace("main", 15, "g", 1);
  GroundTruth.emplace("main", 15, "i", 0);
  GroundTruth.emplace("main", 16, "g", 1);
  GroundTruth.emplace("main", 17, "g", 2);
  GroundTruth.emplace("main", 17, "i", 4);
  GroundTruth.emplace("main", 18, "g", 2);
  GroundTruth.emplace("main", 18, "i", 4);
  compareResults(Results, GroundTruth);
}

TEST_F(IDELinearConstantAnalysisDotTest, HandleGlobalsTest_08) {
  auto Results = doAnalysis("global_08.dbg.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("_Z3bari", 7, "b", 2);
  GroundTruth.emplace("_Z3bari", 7, "g", 2);
  GroundTruth.emplace("_Z3bari", 8, "b", 2);
  GroundTruth.emplace("_Z3bari", 8, "g", 2);

  GroundTruth.emplace("_Z3bazi", 3, "g", 2);
  GroundTruth.emplace("_Z3bazi", 3, "c", 3);
  GroundTruth.emplace("_Z3bazi", 4, "g", 2);
  GroundTruth.emplace("_Z3bazi", 4, "c", 3);

  GroundTruth.emplace("_Z3fooi", 11, "g", 2);
  GroundTruth.emplace("_Z3fooi", 11, "a", 1);
  GroundTruth.emplace("_Z3fooi", 12, "g", 2);
  GroundTruth.emplace("_Z3fooi", 12, "a", 1);

  GroundTruth.emplace("main", 16, "g", 2);
  GroundTruth.emplace("main", 17, "g", 2);
  GroundTruth.emplace("main", 17, "i", 0);
  GroundTruth.emplace("main", 18, "g", 2);
  GroundTruth.emplace("main", 19, "g", 2);
  compareResults(Results, GroundTruth);
}
