#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIDESolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */

class IDELinearConstantAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/linear_constant/";
  const std::vector<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  IDELinearConstantAnalysis *LCAProblem;

  IDELinearConstantAnalysisTest() = default;
  virtual ~IDELinearConstantAnalysisTest() = default;

  void Initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles);
    IRDB->preprocessIR();
    TH = new LLVMTypeHierarchy(*IRDB);
    ICFG = new LLVMBasedICFG(*TH, *IRDB, WalkerStrategy::Pointer,
                             ResolveStrategy::OTF, EntryPoints);
    LCAProblem = new IDELinearConstantAnalysis(*ICFG, EntryPoints);
  }

  void SetUp() override {
    initializeLogger(false);
    ValueAnnotationPass::resetValueID();
  }

  void TearDown() override {
    PAMM_FACTORY;
    delete IRDB;
    delete TH;
    delete ICFG;
    delete LCAProblem;
    PAMM_RESET;
  }

  void compareResults(
      const std::map<int, int64_t> &groundTruth,
      LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> &solver) {
    std::map<int, int64_t> results;
    for (auto exit : ICFG->getExitPointsOf(ICFG->getMethod("main"))) {
      for (auto res : solver.resultsAt(exit, true)) {
        int id = std::stoi(getMetaDataID(res.first));
        // std::cout << "\n\nValue: " << res.second << " at: " << id <<
        // std::endl; res.first->print(llvm::outs());
        results.insert(std::pair<int, int64_t>(id, res.second));
      }
    }
    // std::cout << std::endl;
    EXPECT_THAT(results, ::testing::ContainerEq(groundTruth));
  }
};

/* ============== BASIC TESTS ============== */
TEST_F(IDELinearConstantAnalysisTest, HandleBasicTest_01) {
  Initialize({pathToLLFiles + "basic_01.ll"});
  LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
      *LCAProblem, false);
  llvmlcasolver.solve();
  const std::map<int, int64_t> gt = {{0, 0}, {1, 13}};
  compareResults(gt, llvmlcasolver);
}

TEST_F(IDELinearConstantAnalysisTest, HandleBasicTest_02) {
  Initialize({pathToLLFiles + "basic_02.ll"});
  LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
      *LCAProblem, false);
  llvmlcasolver.solve();
  const std::map<int, int64_t> gt = {{0, 0}, {1, 17}};
  compareResults(gt, llvmlcasolver);
}

TEST_F(IDELinearConstantAnalysisTest, HandleBasicTest_03) {
  Initialize({pathToLLFiles + "basic_03.ll"});
  LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
      *LCAProblem, false);
  llvmlcasolver.solve();
  const std::map<int, int64_t> gt = {{0, 0}, {1, 14}, {2, 14}, {6, 14}};
  compareResults(gt, llvmlcasolver);
}

TEST_F(IDELinearConstantAnalysisTest, HandleBasicTest_04) {
  Initialize({pathToLLFiles + "basic_04.ll"});
  LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
      *LCAProblem, false);
  llvmlcasolver.solve();
  const std::map<int, int64_t> gt = {
      {0, 0}, {1, 14}, {2, 20}, {7, 14}, {8, 20}};
  compareResults(gt, llvmlcasolver);
}

TEST_F(IDELinearConstantAnalysisTest, HandleBasicTest_05) {
  Initialize({pathToLLFiles + "basic_05.ll"});
  LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
      *LCAProblem, false);
  llvmlcasolver.solve();
  const std::map<int, int64_t> gt = {{0, 0}, {1, 3},  {2, 14},
                                     {5, 3}, {6, 12}, {7, 14}};
  compareResults(gt, llvmlcasolver);
}

TEST_F(IDELinearConstantAnalysisTest, HandleBasicTest_06) {
  Initialize({pathToLLFiles + "basic_06.ll"});
  LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
      *LCAProblem, false);
  llvmlcasolver.solve();
  const std::map<int, int64_t> gt = {{0, 16}};
  compareResults(gt, llvmlcasolver);
}

TEST_F(IDELinearConstantAnalysisTest, HandleBranchTest_01) {
  Initialize({pathToLLFiles + "branch_01.ll"});
  LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
      *LCAProblem, false);
  llvmlcasolver.solve();
  const std::map<int, int64_t> gt = {
      {1, 0}, {2, LCAProblem->bottomElement()}, {8, 10}, {9, 12}};
  compareResults(gt, llvmlcasolver);
}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
