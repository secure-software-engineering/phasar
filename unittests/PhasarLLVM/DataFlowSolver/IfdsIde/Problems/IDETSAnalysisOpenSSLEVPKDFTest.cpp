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
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFCTXDescription.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h>
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
      "build/test/llvm_test_code/openssl/key_derivation/";
  const std::set<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  LLVMPointsToInfo *PT;
  OpenSSLEVPKDFCTXDescription *OpenSSLEVPKeyDerivationDesc;
  OpenSSLEVPKDFDescription *OpenSSLEVPKDFDesc;
  IDETypeStateAnalysis *TSProblem, *TSKDFProblem;
  IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
            IDETypeStateAnalysis::m_t, IDETypeStateAnalysis::t_t,
            IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
            IDETypeStateAnalysis::i_t> *llvmtssolver,
      *kdfSolver;

  enum OpenSSLEVPKeyDerivationState {
    TOP = 42,
    UNINIT = 0,
    CTX_ATTACHED = 1,
    PARAM_INIT = 2,
    DERIVED = 3,
    ERROR = 4,
    BOT = 5
  };
  IDETSAnalysisOpenSSLEVPKDFTest() = default;
  virtual ~IDETSAnalysisOpenSSLEVPKDFTest() = default;

  void Initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles, IRDBOptions::WPA);
    TH = new LLVMTypeHierarchy(*IRDB);
    PT = new LLVMPointsToInfo(*IRDB);
    ICFG = new LLVMBasedICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, TH,
                             PT);

    OpenSSLEVPKDFDesc = new OpenSSLEVPKDFDescription();
    TSKDFProblem = new IDETypeStateAnalysis(IRDB, TH, ICFG, PT,
                                            *OpenSSLEVPKDFDesc, EntryPoints);
    kdfSolver =
        new IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
                      IDETypeStateAnalysis::m_t, IDETypeStateAnalysis::t_t,
                      IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
                      IDETypeStateAnalysis::i_t>(*TSKDFProblem);

    OpenSSLEVPKeyDerivationDesc = new OpenSSLEVPKDFCTXDescription(*kdfSolver);
    TSProblem = new IDETypeStateAnalysis(
        IRDB, TH, ICFG, PT, *OpenSSLEVPKeyDerivationDesc, EntryPoints);

    llvmtssolver =
        new IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
                      IDETypeStateAnalysis::m_t, IDETypeStateAnalysis::t_t,
                      IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
                      IDETypeStateAnalysis::i_t>(*TSProblem);
    kdfSolver->solve();
    llvmtssolver->solve();
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
      const std::map<std::size_t, std::map<std::string, int>> &groundTruth) {
    for (auto InstToGroundTruth : groundTruth) {
      auto Inst = IRDB->getInstruction(InstToGroundTruth.first);
      auto GT = InstToGroundTruth.second;
      std::map<std::string, int> results;
      for (auto Result : llvmtssolver->resultsAt(Inst, true)) {
        if (GT.find(getMetaDataID(Result.first)) != GT.end()) {
          results.insert(std::pair<std::string, int>(
              getMetaDataID(Result.first), Result.second));
        }
      }
      EXPECT_EQ(results, GT);
    }
  }
}; // Test Fixture

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation1) {
  Initialize({pathToLLFiles + "key-derivation1_c.ll"});

  // llvmtssolver->printReport();

  std::map<std::size_t, std::map<std::string, int>> gt;

  gt[48] = {{"46", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"20", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[50] = {{"46", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"20", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};

  gt[92] = {{"46", OpenSSLEVPKeyDerivationState::PARAM_INIT},
            {"20", OpenSSLEVPKeyDerivationState::PARAM_INIT},
            {"88", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  gt[98] = {{"95", OpenSSLEVPKeyDerivationState::DERIVED},
            {"46", OpenSSLEVPKeyDerivationState::DERIVED},
            {"20", OpenSSLEVPKeyDerivationState::DERIVED},
            {"88", OpenSSLEVPKeyDerivationState::DERIVED}};
  gt[146] = {{"144", OpenSSLEVPKeyDerivationState::UNINIT},
             {"95", OpenSSLEVPKeyDerivationState::UNINIT},
             {"46", OpenSSLEVPKeyDerivationState::UNINIT},
             {"20", OpenSSLEVPKeyDerivationState::UNINIT},
             {"88", OpenSSLEVPKeyDerivationState::UNINIT}};
  compareResults(gt);
}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
