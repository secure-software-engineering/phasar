#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
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
  static constexpr auto PathToSwiftTestFiles = unittest::PathToSwiftTestFiles;
  // Function - Line Nr - Variable - Value
  using LCACompactResult_t = std::tuple<std::string, std::size_t, std::string,
                                        IDELinearConstantAnalysisDomain::l_t>;
  std::unique_ptr<LLVMProjectIRDB> IRDB;

  void SetUp() override {}

  IDELinearConstantAnalysis::lca_results_t
  doAnalysis(llvm::StringRef LlvmFilePath, bool PrintDump = false) {
    IRDB =
        std::make_unique<LLVMProjectIRDB>(PathToSwiftTestFiles + LlvmFilePath);
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMAliasSet PT(IRDB.get());
    LLVMBasedICFG ICFG(IRDB.get(), CallGraphAnalysisType::OTF, {"main"}, &TH,
                       &PT, Soundness::Soundy, /*IncludeGlobals*/ true);

    auto HasGlobalCtor = IRDB->getFunctionDefinition(
                             LLVMBasedICFG::GlobalCRuntimeModelName) != nullptr;
    IDELinearConstantAnalysis LCAProblem(
        IRDB.get(), &ICFG,
        {HasGlobalCtor ? LLVMBasedICFG::GlobalCRuntimeModelName.str()
                       : "main"});
    IDESolver LCASolver(LCAProblem, &ICFG);
    LCASolver.solve();
    if (PrintDump) {
      IRDB->dump();
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
  auto Results = doAnalysis("basic_swift_01.ll");
  std::set<LCACompactResult_t> GroundTruth;
  GroundTruth.emplace("$s14basic_swift_016MyMainV9simpleAdd1xS2i_tFZ", 14, "a",
                      1);
  GroundTruth.emplace("$s14basic_swift_016MyMainV9simpleAdd1xS2i_tFZ", 14, "x",
                      1);
  GroundTruth.emplace("$s14basic_swift_016MyMainV9simpleAdd1xS2i_tFZ", 14, "b",
                      42);
  compareResults(Results, GroundTruth);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
