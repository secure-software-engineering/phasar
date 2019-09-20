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
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/typestate_analysis/";
  const std::vector<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  CSTDFILEIOTypeStateDescription *CSTDFILEIODesc;
  IDETypeStateAnalysis *TSProblem;
  enum IOSTATE {
    TOP = 42,
    UNINIT = 0,
    OPENED = 1,
    CLOSED = 2,
    ERROR = 3,
    BOT = 4
  };

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
      const std::map<std::size_t, std::map<std::string, int>> &groundTruth,
      LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> &solver) {
    for (auto InstToGroundTruth : groundTruth) {
      auto Inst = IRDB->getInstruction(InstToGroundTruth.first);
      auto GT = InstToGroundTruth.second;
      std::map<std::string, int> results;
      for (auto Result : solver.resultsAt(Inst, true)) {
        if (GT.find(getMetaDataID(Result.first)) != GT.end()) {
          results.insert(std::pair<std::string, int>(
              getMetaDataID(Result.first), Result.second));
        }
      }
      EXPECT_EQ(results, GT);
    }
  }
}; // Test Fixture

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_01) {
  Initialize({pathToLLFiles + "typestate_01_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      {5, {{"3", IOSTATE::UNINIT}}},
      {9, {{"3", IOSTATE::CLOSED}}},
      {7, {{"3", IOSTATE::OPENED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_02) {
  Initialize({pathToLLFiles + "typestate_02_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      {7, {{"3", IOSTATE::OPENED}, {"5", IOSTATE::OPENED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_03) {
  Initialize({pathToLLFiles + "typestate_03_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // Entry in foo()
      {2, {{"foo.0", IOSTATE::OPENED}}},
      // Exit in foo()
      {6,
       {{"foo.0", IOSTATE::CLOSED},
        {"2", IOSTATE::CLOSED},
        {"4", IOSTATE::CLOSED},
        {"8", IOSTATE::CLOSED}}},
      // Exit in main()
      {14,
       {{"2", IOSTATE::CLOSED},
        {"8", IOSTATE::CLOSED},
        {"12", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_04) {
  Initialize({pathToLLFiles + "typestate_04_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // At exit in foo()
      {6, {{"2", IOSTATE::OPENED}, {"8", IOSTATE::OPENED}}},
      // At exit in main()
      {14, {{"2", IOSTATE::CLOSED}, {"8", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_05) {
  Initialize({pathToLLFiles + "typestate_05_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // Before if statement
      {10,
       {{"4", IOSTATE::OPENED},
        {"6", IOSTATE::OPENED},
        {"11", IOSTATE::OPENED}}},
      // Inside if statement at last instruction
      {13,
       {{"4", IOSTATE::CLOSED},
        {"6", IOSTATE::CLOSED},
        {"11", IOSTATE::CLOSED}}},
      // After if statement
      {14, {{"4", IOSTATE::BOT}, {"6", IOSTATE::BOT}, {"11", IOSTATE::BOT}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, DISABLED_HandleTypeState_06) {
  // This test fails due to imprecise points-to information
  Initialize({pathToLLFiles + "typestate_06_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // Before first fopen()
      {8, {{"5", IOSTATE::UNINIT}, {"6", IOSTATE::UNINIT}}},
      // Before storing the result of the first fopen()
      {9,
       {{"5", IOSTATE::UNINIT},
        {"6", IOSTATE::UNINIT},
        // Return value of first fopen()
        {"8", IOSTATE::OPENED}}},
      // Before second fopen()
      {10,
       {{"5", IOSTATE::OPENED},
        {"6", IOSTATE::UNINIT},
        {"8", IOSTATE::OPENED}}},
      // Before storing the result of the second fopen()
      {11,
       {{"5", IOSTATE::OPENED},
        {"6", IOSTATE::UNINIT},
        // Return value of second fopen()
        {"10", IOSTATE::OPENED}}},
      // Before fclose()
      {13,
       {{"5", IOSTATE::OPENED},
        {"6", IOSTATE::OPENED},
        {"12", IOSTATE::OPENED}}},
      // After if statement
      {14, {{"5", IOSTATE::CLOSED}, {"6", IOSTATE::OPENED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_07) {
  Initialize({pathToLLFiles + "typestate_07_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // In foo()
      {6,
       {{"foo.0", IOSTATE::CLOSED},
        {"2", IOSTATE::CLOSED},
        {"8", IOSTATE::CLOSED}}},
      // At fclose()
      {11, {{"8", IOSTATE::UNINIT}, {"10", IOSTATE::UNINIT}}},
      // After fclose()
      {12, {{"8", IOSTATE::ERROR}, {"10", IOSTATE::ERROR}}},
      // After fopen()
      {13,
       {{"8", IOSTATE::ERROR},
        {"10", IOSTATE::ERROR},
        {"12", IOSTATE::OPENED}}},
      // After store
      {14,
       {{"8", IOSTATE::OPENED},
        {"10", IOSTATE::OPENED},
        {"12", IOSTATE::OPENED}}},
      // At exit in main()
      {16, {{"2", IOSTATE::CLOSED}, {"8", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_08) {
  Initialize({pathToLLFiles + "typestate_08_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // At exit in foo()
      {6, {{"2", IOSTATE::OPENED}}},
      // At exit in main()
      {11, {{"2", IOSTATE::OPENED}, {"8", IOSTATE::UNINIT}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_09) {
  Initialize({pathToLLFiles + "typestate_09_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // At exit in foo()
      {8, {{"4", IOSTATE::OPENED}, {"10", IOSTATE::OPENED}}},
      // At exit in main()
      {18, {{"4", IOSTATE::CLOSED}, {"10", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_10) {
  Initialize({pathToLLFiles + "typestate_10_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // At exit in bar()
      {4, {{"2", IOSTATE::UNINIT}}},
      // At exit in foo()
      {11,
       {{"2", IOSTATE::OPENED},
        {"5", IOSTATE::OPENED},
        {"13", IOSTATE::OPENED}}},
      // At exit in main()
      {19,
       {{"2", IOSTATE::CLOSED},
        {"5", IOSTATE::CLOSED},
        {"13", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_11) {
  Initialize({pathToLLFiles + "typestate_11_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // At exit in bar()
      {6,
       {{"2", IOSTATE::CLOSED},
        {"7", IOSTATE::CLOSED},
        {"13", IOSTATE::CLOSED}}},
      // At exit in foo()
      {11,
       {{"2", IOSTATE::OPENED},
        {"7", IOSTATE::OPENED},
        {"13", IOSTATE::OPENED}}},
      // At exit in main()
      {19,
       {{"2", IOSTATE::CLOSED},
        {"7", IOSTATE::CLOSED},
        {"13", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_12) {
  Initialize({pathToLLFiles + "typestate_12_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // At exit in bar()
      {6, {{"2", IOSTATE::OPENED}, {"10", IOSTATE::OPENED}}},
      // At exit in foo()
      {8, {{"2", IOSTATE::OPENED}, {"10", IOSTATE::OPENED}}},
      // At exit in main()
      {16, {{"2", IOSTATE::CLOSED}, {"10", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_13) {
  Initialize({pathToLLFiles + "typestate_13_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // Before first fclose()
      {8, {{"3", IOSTATE::OPENED}}},
      // Before second fclose()
      {10, {{"3", IOSTATE::CLOSED}}},
      // At exit in main()
      {11, {{"3", IOSTATE::ERROR}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_14) {
  Initialize({pathToLLFiles + "typestate_14_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // Before first fopen()
      {7, {{"5", IOSTATE::UNINIT}}},
      // Before second fopen()
      {9, {{"5", IOSTATE::OPENED}}},
      // After second store
      {11,
       {{"5", IOSTATE::OPENED},
        {"7", IOSTATE::OPENED},
        {"9", IOSTATE::OPENED}}},
      // At exit in main()
      {11,
       {{"5", IOSTATE::CLOSED},
        {"7", IOSTATE::CLOSED},
        {"9", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_15) {
  Initialize({pathToLLFiles + "typestate_15_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // After store of ret val of first fopen()
      {9,
       {{"5", IOSTATE::OPENED},
        {"7", IOSTATE::OPENED},
        {"9", IOSTATE::OPENED},
        {"11", IOSTATE::OPENED},
        {"13", IOSTATE::OPENED}}},
      // After first fclose()
      {11,
       {{"5", IOSTATE::CLOSED},
        {"7", IOSTATE::CLOSED},
        {"9", IOSTATE::CLOSED},
        {"11", IOSTATE::CLOSED},
        {"13", IOSTATE::CLOSED}}},
      // After second fopen() but before storing ret val
      {12,
       {{"5", IOSTATE::CLOSED},
        {"7", IOSTATE::CLOSED},
        {"9", IOSTATE::CLOSED},
        {"11", IOSTATE::OPENED},
        {"13", IOSTATE::CLOSED}}},
      // After storing ret val of second fopen()
      {13,
       {{"5", IOSTATE::OPENED},
        {"7", IOSTATE::OPENED},
        {"9", IOSTATE::OPENED},
        {"11", IOSTATE::OPENED},
        {"13", IOSTATE::OPENED}}},
      // At exit in main()
      {15,
       {{"5", IOSTATE::CLOSED},
        {"7", IOSTATE::CLOSED},
        {"9", IOSTATE::CLOSED},
        {"11", IOSTATE::CLOSED},
        {"13", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_16) {
  Initialize({pathToLLFiles + "typestate_16_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // At exit in foo()
      {16, {{"2", IOSTATE::CLOSED}, {"18", IOSTATE::CLOSED}}},
      // At exit in main()
      {24, {{"2", IOSTATE::CLOSED}, {"18", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

// TODO: Check this case again!
TEST_F(IDETypeStateAnalysisTest, HandleTypeState_17) {
  Initialize({pathToLLFiles + "typestate_17_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // Before fgetc()
      {17,
       {{"2", IOSTATE::BOT},
        {"9", IOSTATE::BOT},
        {"13", IOSTATE::BOT},
        {"16", IOSTATE::BOT}}},
      // After fgetc()
      {18,
       {{"2", IOSTATE::BOT},
        {"9", IOSTATE::BOT},
        {"13", IOSTATE::BOT},
        {"16", IOSTATE::BOT}}},
      // At exit in main()
      {22,
       {{"2", IOSTATE::BOT},
        {"9", IOSTATE::BOT},
        {"13", IOSTATE::BOT},
        {"16", IOSTATE::BOT}}}};
  compareResults(gt, llvmtssolver);
}

TEST_F(IDETypeStateAnalysisTest, HandleTypeState_18) {
  Initialize({pathToLLFiles + "typestate_18_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      // At exit in foo()
      {17, {{"2", IOSTATE::CLOSED}, {"19", IOSTATE::CLOSED}}},
      // At exit in main()
      {25, {{"2", IOSTATE::CLOSED}, {"19", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

// TODO: Check this case again!
TEST_F(IDETypeStateAnalysisTest, HandleTypeState_19) {
  Initialize({pathToLLFiles + "typestate_19_c.ll"});
  LLVMIDESolver<const llvm::Value *, int, LLVMBasedICFG &> llvmtssolver(
      *TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      {11, {{"8", IOSTATE::UNINIT}}},
      {14, {{"8", IOSTATE::BOT}}},
      // At exit in main()
      {25, {{"2", IOSTATE::CLOSED}, {"8", IOSTATE::CLOSED}}}};
  compareResults(gt, llvmtssolver);
}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
