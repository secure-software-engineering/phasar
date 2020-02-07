/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKeyDerivationTypeStateDescription.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETSAnalysisOpenSSLEVPKDFTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/openssl/";
  const std::set<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  LLVMPointsToInfo *PT;
  OpenSSLEVPKeyDerivationTypeStateDescription *OpenSSLEVPKeyDerivationDesc;
  IDETypeStateAnalysis *TSProblem;

  enum OpenSSLEVPKeyDerivationState {
    TOP = 42,
    UNINIT = 0,
    KDF_FETCHED = 1,
    CTX_ATTACHED = 2,
    PARAM_INIT = 3,
    DERIVED = 4,
    ERROR = 5,
    BOT = 6
  };
  IDETSAnalysisOpenSSLEVPKDFTest() = default;
  virtual ~IDETSAnalysisOpenSSLEVPKDFTest() = default;

  void Initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles, IRDBOptions::WPA);
    TH = new LLVMTypeHierarchy(*IRDB);
    PT = new LLVMPointsToInfo(*IRDB);
    ICFG = new LLVMBasedICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, TH,
                             PT);
    OpenSSLEVPKeyDerivationDesc =
        new OpenSSLEVPKeyDerivationTypeStateDescription();
    TSProblem = new IDETypeStateAnalysis(
        IRDB, TH, ICFG, PT, *OpenSSLEVPKeyDerivationDesc, EntryPoints);
  }

  void SetUp() override {
    boost::log::core::get()->set_logging_enabled(false);
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
   * a string since Argument ID's are not integer type (e.g. main.0 for
   argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  void compareResults(
      const std::map<std::size_t, std::map<std::string, int>> &groundTruth,
      IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
                IDETypeStateAnalysis::m_t, IDETypeStateAnalysis::t_t,
                IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
                IDETypeStateAnalysis::i_t> &solver) {
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

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, HandleTypeState_01) {
  Initialize({pathToLLFiles + "openssl_program1_c.ll"});
  IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
            IDETypeStateAnalysis::m_t, IDETypeStateAnalysis::t_t,
            IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
            IDETypeStateAnalysis::i_t>
      llvmtssolver(*TSProblem);

  llvmtssolver.solve();
  const std::map<std::size_t, std::map<std::string, int>> gt = {
      {61, {{"60", OpenSSLEVPKeyDerivationState::KDF_FETCHED}}},
      {62, {{"34", OpenSSLEVPKeyDerivationState::KDF_FETCHED}}},
      {69,
       {{"67", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
        {"35", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}}}};
  // TODO add more GT values
  compareResults(gt, llvmtssolver);
}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
