#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDEVariabilityTabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <sstream>

#include <tuple>
#define DELETE(x)                                                              \
  if (x) {                                                                     \
    delete (x);                                                                \
    (x) = nullptr;                                                             \
  }

using namespace psr;

/* ============== TEST FIXTURE ============== */
class VariabilityCFGTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/variability/";
  const std::set<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB = nullptr;
  LLVMTypeHierarchy *TH = nullptr;
  LLVMPointsToInfo *PT = nullptr;
  LLVMBasedVariationalICFG *VICFG = nullptr;

  void SetUp() override { boost::log::core::get()->set_logging_enabled(false); }

  void initialize(const std::string &llvmFilePath) {

    IRDB = new ProjectIRDB({pathToLLFiles + llvmFilePath}, IRDBOptions::WPA);
    ValueAnnotationPass::resetValueID();
    TH = new LLVMTypeHierarchy(*IRDB);
    PT = new LLVMPointsToInfo(*IRDB);
    VICFG = new LLVMBasedVariationalICFG(*IRDB, CallGraphAnalysisType::OTF,
                                         EntryPoints, TH, PT);
  }
  bool doAnalysis(const llvm::Instruction *currInst,
                  const llvm::Instruction *succInst, z3::expr &ret) {

    return VICFG->isPPBranchTarget(currInst, succInst, ret);
  }

  void TearDown() override {
    DELETE(VICFG);
    DELETE(TH);
    DELETE(PT);
    DELETE(IRDB);
  }

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  void compareResults(const z3::expr &groundTruth, const z3::expr &result) {
    std::stringstream s1, s2;
    s1 << groundTruth;
    s2 << result;
    // find better way of comparing
    EXPECT_EQ(s1.str(), s2.str());
  }
}; // Test Fixture

TEST_F(VariabilityCFGTest, twovariables_desugared) {
  initialize("twovariables_desugared_c.ll");
  z3::expr exp = VICFG->getTrueCondition();
  auto currInst = IRDB->getInstruction(9);
  auto succInst = IRDB->getInstruction(10);
  ASSERT_NE(currInst, nullptr);
  ASSERT_NE(succInst, nullptr);
  EXPECT_TRUE(VICFG->isBranchTarget(currInst, succInst));
  EXPECT_TRUE(doAnalysis(currInst, succInst, exp));
  std::cout << exp << std::endl;
  auto &ctx = VICFG->getContext();
  compareResults(exp, ctx.bool_const("B_defined"));
}
// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}