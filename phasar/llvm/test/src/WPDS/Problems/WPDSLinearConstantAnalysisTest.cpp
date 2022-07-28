// to use unique_ptr
#include <memory>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSLinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Solver/WPDSSolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */
class WPDSLinearConstantAnalysisTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles = "llvm_test_code/linear_constant/";
  const std::set<std::string> EntryPoints = {"main"};

  unique_ptr<ProjectIRDB> IRDB;
  unique_ptr<LLVMTypeHierarchy> TH;
  unique_ptr<LLVMPointsToInfo> PT;
  unique_ptr<LLVMBasedICFG> ICFG;
  unique_ptr<WPDSLinearConstantAnalysis> LCAProblem;

  WPDSLinearConstantAnalysisTest() = default;
  ~WPDSLinearConstantAnalysisTest() override = default;

  void initialize(const std::vector<std::string> &IRFiles) {
    IRDB = make_unique<ProjectIRDB>(IRFiles, IRDBOptions::WPA);
    TH = make_unique<LLVMTypeHierarchy>(*IRDB);
    PT = make_unique<LLVMPointsToSet>(*IRDB);
    ICFG = make_unique<LLVMBasedICFG>(*IRDB, CallGraphAnalysisType::OTF,
                                      EntryPoints, TH.get(), PT.get());
    // LCAProblem = make_unique<WPDSLinearConstantAnalysis>(
    //    *ICFG, *TH, *IRDB, WPDSType::FWPDS, WPDSSearchDirection::BACKWARD);
  }

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {
    PAMM_GET_INSTANCE;
    PAMM_RESET;
  }

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  // void compareResults(
  //     const std::map<std::string, int64_t> &groundTruth,
  //     LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> &solver) {
  //   std::map<std::string, int64_t> results;
  //   for (auto M : IRDB->getAllModules()) {
  //     for (auto &F : *M) {
  //       for (auto exit : ICFG->getExitPointsOf(&F)) {
  //         for (auto res : solver.resultsAt(exit, true)) {
  //           results.insert(std::pair<std::string, int64_t>(
  //               getMetaDataID(res.first), res.second));
  //         }
  //       }
  //     }
  //   }
  //   EXPECT_EQ(results, groundTruth);
  // }
}; // Test Fixture

// /* ============== BASIC TESTS ============== */
// TEST_F(WPDSLinearConstantAnalysisTest, HandleBasicTest_01) {
//   Initialize({pathToLLFiles + "basic_01.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {{"0", 0}, {"1", 13}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleBasicTest_02) {
//   Initialize({pathToLLFiles + "basic_02.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {{"0", 0}, {"1", 17}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleBasicTest_03) {
//   Initialize({pathToLLFiles + "basic_03.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {
//       {"0", 0}, {"1", 14}, {"2", 14}, {"8", 14}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleBasicTest_04) {
//   Initialize({pathToLLFiles + "basic_04.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {
//       {"0", 0}, {"1", 14}, {"2", 20}, {"10", 14}, {"11", 20}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleBasicTest_05) {
//   Initialize({pathToLLFiles + "basic_05.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {{"0", 0}, {"1", 3},  {"2", 14},
//                                              {"7", 3}, {"8", 12}, {"9", 14}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleBasicTest_06) {
//   Initialize({pathToLLFiles + "basic_06.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {{"1", 16}};
//   compareResults(gt, llvmlcasolver);
// }

// /* ============== BRANCH TESTS ============== */
// TEST_F(WPDSLinearConstantAnalysisTest, HandleBranchTest_01) {
//   Initialize({pathToLLFiles + "branch_01.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {
//       {"1", 0}, {"2", LCAProblem->bottomElement()}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleBranchTest_02) {
//   // Probably a bad example/style, since variable i is possibly unitialized
//   Initialize({pathToLLFiles + "branch_02.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {{"1", 0}, {"2", 10}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleBranchTest_03) {
//   Initialize({pathToLLFiles + "branch_03.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {{"1", 0}, {"2", 30}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleBranchTest_04) {
//   Initialize({pathToLLFiles + "branch_04.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {{"1", 0},
//                                              {"2", 10},
//                                              {"3",
//                                              LCAProblem->bottomElement()},
//                                              {"12", 10},
//                                              {"13", 20}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleBranchTest_05) {
//   Initialize({pathToLLFiles + "branch_05.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {
//       {"1", 0},  {"2", 10},  {"3", LCAProblem->bottomElement()},
//       {"8", 10}, {"13", 10}, {"14", 20}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleBranchTest_06) {
//   Initialize({pathToLLFiles + "branch_06.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {{"1", 0},
//                                              {"2", 10},
//                                              {"3",
//                                              LCAProblem->bottomElement()},
//                                              {"8", 10},
//                                              {"9", 20}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleBranchTest_07) {
//   Initialize({pathToLLFiles + "branch_07.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {
//       {"1", 0},  {"2", 10}, {"3", LCAProblem->bottomElement()},
//       {"8", 10}, {"9", 30}, {"14", 10},
//       {"15", 12}};
//   compareResults(gt, llvmlcasolver);
// }

// /* ============== CALL TESTS ============== */
// TEST_F(WPDSLinearConstantAnalysisTest, HandleCallTest_01) {
//   Initialize({pathToLLFiles + "call_01.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {
//       {"0", 42}, {"1", 42},  {"5", 42},        {"8", 0},
//       {"9", 42}, {"13", 42}, {"_Z3fooi.0", 42}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleCallTest_02) {
//   Initialize({pathToLLFiles + "call_02.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {
//       {"0", 2},  {"3", 2},   {"4", 42},       {"6", 0},
//       {"7", 42}, {"10", 42}, {"_Z3fooi.0", 2}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleCallTest_03) {
//   Initialize({pathToLLFiles + "call_03.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {{"1", 0}, {"2", 42}, {"5", 42}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleCallTest_04) {
//   Initialize({pathToLLFiles + "call_04.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {{"1", 0}, {"2", 10}, {"6", 42}};
//   compareResults(gt, llvmlcasolver);
// }

// TEST_F(WPDSLinearConstantAnalysisTest, HandleCallTest_05) {
//   Initialize({pathToLLFiles + "call_05.dbg.ll"});
//   LLVMIDESolver<const llvm::Value *, int64_t, LLVMBasedICFG &> llvmlcasolver(
//       *LCAProblem);
//   llvmlcasolver.solve();
//   const std::map<std::string, int64_t> gt = {
//       {"0", 0},
//       {"1", LCAProblem->bottomElement()},
//       {"3", LCAProblem->bottomElement()},
//       {"10", LCAProblem->bottomElement()},
//       {"main.0", LCAProblem->bottomElement()}};
//   compareResults(gt, llvmlcasolver);
// }
