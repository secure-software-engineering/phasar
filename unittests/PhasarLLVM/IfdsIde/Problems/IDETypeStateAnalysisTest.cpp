#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IDETypeStateAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIDESolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETypeStateAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/typestate_analysis/";
  const std::vector<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  CSTDFILEIOTypeStateDescription *CSTDFILEIODesc;
  IDETypeStateAnalysis *TSProblem;

  IDETypeStateAnalysisTest() = default;
  virtual ~IDETypeStateAnalysisTest() = default;

  void Initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles);
    IRDB->preprocessIR();
    TH = new LLVMTypeHierarchy(*IRDB);
    ICFG =
        new LLVMBasedICFG(*TH, *IRDB, CallGraphAnalysisType::OTF, EntryPoints);
    CSTDFILEIODesc = new CSTDFILEIOTypeStateDescription();
    TSProblem = new IDETypeStateAnalysis(*ICFG, *TH, *IRDB, *CSTDFILEIODesc,
                                         EntryPoints);
  }

  void SetUp() override {
    bl::core::get()->set_logging_enabled(false);
    ValueAnnotationPass::resetValueID();
  }

  void TearDown() override {
    delete IRDB;
    delete TH;
    delete ICFG;
    delete TSProblem;
  }

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  void compareResults(
      const std::map<std::string, int> &groundTruth,
      LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> &solver) {
    std::map<std::string, int> results;
    for (auto M : IRDB->getAllModules()) {
      for (auto &F : *M) {
        for (auto exit : ICFG->getExitPointsOf(&F)) {
          for (auto res : solver.resultsAt(exit, true)) {
            results.insert(std::pair<std::string, int>(getMetaDataID(res.first),
                                                       res.second));
          }
        }
      }
    }
    EXPECT_EQ(results, groundTruth);
  }
}; // Test Fixture

/* ============== BASIC TESTS ============== */
TEST_F(IDETypeStateAnalysisTest, HandleTypeState_01) {
  Initialize({pathToLLFiles + "typestate_01_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem, false, false);

  llvmtssolver.solve();
  const std::map<std::string, int> gt = {
      {"5", CSTDFILEIOTypeStateDescription::CSTDFILEIOState::BOT},
      {"9", CSTDFILEIOTypeStateDescription::CSTDFILEIOState::BOT},
      {"15", CSTDFILEIOTypeStateDescription::CSTDFILEIOState::CLOSED}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_02) {
  Initialize({pathToLLFiles + "typestate_02_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem, false, false);

  llvmtssolver.solve();
  const std::map<std::string, int> gt = {
      {"3", CSTDFILEIOTypeStateDescription::CSTDFILEIOState::OPENED},
      {"5", CSTDFILEIOTypeStateDescription::CSTDFILEIOState::OPENED}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_03) {
  Initialize({pathToLLFiles + "typestate_03_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem, false, false);

  llvmtssolver.solve();
  const std::map<std::string, int> gt = {
      {"4", CSTDFILEIOTypeStateDescription::CSTDFILEIOState::CLOSED},
      {"11", CSTDFILEIOTypeStateDescription::CSTDFILEIOState::CLOSED},
      {"13", CSTDFILEIOTypeStateDescription::CSTDFILEIOState::CLOSED}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_04) {
  Initialize({pathToLLFiles + "typestate_04_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem, true, false);

  llvmtssolver.solve();
  // const std::map<std::string, State> gt = {{"0", 0}, {"1", 13}};
  // compareResults(gt, llvmtssolver);
}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
