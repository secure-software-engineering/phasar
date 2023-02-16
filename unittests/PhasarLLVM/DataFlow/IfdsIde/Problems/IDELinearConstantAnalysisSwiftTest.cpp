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
  const std::string PathToSwiftTestFiles =
      unittest::PathToSwiftTestFiles + "linear_constant/";
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
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 11, "i", 14);
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 11, "k", 34);
  GroundTruth.emplace("$s8basic_046MyMainV10addWrapperyS2iFZ", 11, "x", 14);
  compareResults(Results, GroundTruth);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
