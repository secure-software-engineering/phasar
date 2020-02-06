#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETypeStateAnalysisOpenSSLTest : public ::testing::Test {
  // protected:
  //   const std::string pathToLLFiles =
  //       PhasarConfig::getPhasarConfig().PhasarDirectory() +
  //       "build/test/llvm_test_code/typestate_analysis/";
  //   const std::set<std::string> EntryPoints = {"main"};

  //   ProjectIRDB *IRDB;
  //   LLVMTypeHierarchy *TH;
  //   LLVMBasedICFG *ICFG;
  //   LLVMPointsToInfo *PT;
  //   CSTDFILEIOTypeStateDescription *CSTDFILEIODesc;
  //   IDETypeStateAnalysis *TSProblem;
  //   enum IOSTATE {
  //     TOP = 42,
  //     UNINIT = 0,
  //     OPENED = 1,
  //     CLOSED = 2,
  //     ERROR = 3,
  //     BOT = 4
  //   };

  //   IDETypeStateAnalysisOpenSSLTest() = default;
  //   virtual ~IDETypeStateAnalysisOpenSSLTest() = default;

  //   void Initialize(const std::vector<std::string> &IRFiles) {
  //     IRDB = new ProjectIRDB(IRFiles, IRDBOptions::WPA);
  //     TH = new LLVMTypeHierarchy(*IRDB);
  //     PT = new LLVMPointsToInfo(*IRDB);
  //     ICFG =
  //         new LLVMBasedICFG(*TH, *IRDB, CallGraphAnalysisType::OTF,
  //         EntryPoints);
  //     CSTDFILEIODesc = new CSTDFILEIOTypeStateDescription();
  //     TSProblem = new IDETypeStateAnalysis(IRDB, TH, ICFG, PT,
  //     *CSTDFILEIODesc,
  //                                          EntryPoints);
  //   }

  //   void SetUp() override {
  //     boost::log::core::get()->set_logging_enabled(false);
  //     ValueAnnotationPass::resetValueID();
  //   }

  //   void TearDown() override {
  //     delete IRDB;
  //     delete TH;
  //     delete ICFG;
  //     delete TSProblem;
  //   }

  //   /**
  //    * We map instruction id to value for the ground truth. ID has to be
  //    * a string since Argument ID's are not integer type (e.g. main.0 for
  //    argc).
  //    * @param groundTruth results to compare against
  //    * @param solver provides the results
  //    */
  //   void compareResults(
  //       const std::map<std::size_t, std::map<std::string, int>> &groundTruth,
  //       IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
  //                 IDETypeStateAnalysis::f_t, IDETypeStateAnalysis::t_t,
  //                 IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
  //                 IDETypeStateAnalysis::i_t> &solver) {
  //     for (auto InstToGroundTruth : groundTruth) {
  //       auto Inst = IRDB->getInstruction(InstToGroundTruth.first);
  //       auto GT = InstToGroundTruth.second;
  //       std::map<std::string, int> results;
  //       for (auto Result : solver.resultsAt(Inst, true)) {
  //         if (GT.find(getMetaDataID(Result.first)) != GT.end()) {
  //           results.insert(std::pair<std::string, int>(
  //               getMetaDataID(Result.first), Result.second));
  //         }
  //       }
  //       EXPECT_EQ(results, GT);
  //     }
  //   }
}; // Test Fixture

// TEST_F(IDETypeStateAnalysisOpenSSLTest, HandleTypeState_01) {
//   Initialize({pathToLLFiles + "typestate_01_c.ll"});
//   IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
//             IDETypeStateAnalysis::f_t, IDETypeStateAnalysis::t_t,
//             IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
//             IDETypeStateAnalysis::i_t>
//       llvmtssolver(*TSProblem);

//   llvmtssolver.solve();
//   const std::map<std::size_t, std::map<std::string, int>> gt = {
//       {5, {{"3", IOSTATE::UNINIT}}},
//       {9, {{"3", IOSTATE::CLOSED}}},
//       {7, {{"3", IOSTATE::OPENED}}}};
//   compareResults(gt, llvmtssolver);
// }

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
