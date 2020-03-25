/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "gtest/gtest.h"

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
            IDETypeStateAnalysis::f_t, IDETypeStateAnalysis::t_t,
            IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
            IDETypeStateAnalysis::i_t> *llvmtssolver,
      *kdfSolver;

  enum OpenSSLEVPKeyDerivationState {
    TOP = 42,
    UNINIT = 5,
    CTX_ATTACHED = 1,
    PARAM_INIT = 2,
    DERIVED = 3,
    ERROR = 4,
    BOT = 0
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
                      IDETypeStateAnalysis::f_t, IDETypeStateAnalysis::t_t,
                      IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
                      IDETypeStateAnalysis::i_t>(*TSKDFProblem);

    OpenSSLEVPKeyDerivationDesc = new OpenSSLEVPKDFCTXDescription(*kdfSolver);
    TSProblem = new IDETypeStateAnalysis(
        IRDB, TH, ICFG, PT, *OpenSSLEVPKeyDerivationDesc, EntryPoints);

    llvmtssolver =
        new IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
                      IDETypeStateAnalysis::f_t, IDETypeStateAnalysis::t_t,
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
        if (Result.second != OpenSSLEVPKeyDerivationState::BOT &&
            GT.count(getMetaDataID(Result.first))) {
          results.insert(std::pair<std::string, int>(
              getMetaDataID(Result.first), Result.second));
        }
      }
      EXPECT_EQ(results, GT) << " at " << llvmIRToShortString(Inst);
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

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation2) {
  Initialize({pathToLLFiles + "key-derivation2_c.ll"});

  std::map<std::size_t, std::map<std::string, int>> gt;
  // gt[40] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT}}; // killed by
  // null-initialization
  // gt[57] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT}}; // killed by
  // null-initialization
  gt[60] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[105] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"103", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[106] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  gt[112] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"110", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  gt[113] = {{"22", OpenSSLEVPKeyDerivationState::DERIVED},
             {"58", OpenSSLEVPKeyDerivationState::DERIVED},
             {"103", OpenSSLEVPKeyDerivationState::DERIVED},
             {"110", OpenSSLEVPKeyDerivationState::DERIVED}};
  gt[160] = {{"22", OpenSSLEVPKeyDerivationState::DERIVED},
             {"58", OpenSSLEVPKeyDerivationState::DERIVED},
             {"103", OpenSSLEVPKeyDerivationState::DERIVED},
             {"110", OpenSSLEVPKeyDerivationState::DERIVED},
             {"159", OpenSSLEVPKeyDerivationState::DERIVED}};
  gt[161] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT},
             {"58", OpenSSLEVPKeyDerivationState::UNINIT},
             {"103", OpenSSLEVPKeyDerivationState::UNINIT},
             {"110", OpenSSLEVPKeyDerivationState::UNINIT},
             {"159", OpenSSLEVPKeyDerivationState::UNINIT}};
  // Fails due to merge conflicts: ID43 and ID162 have both value UNINIT on 22,
  // but it is implicit at ID43, so merging gives BOT

  // gt[164] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT}};
  compareResults(gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation3) {
  Initialize({pathToLLFiles + "key-derivation3_c.ll"});
  std::map<std::size_t, std::map<std::string, int>> gt;

  // gt[56] = {{"21", OpenSSLEVPKeyDerivationState::UNINIT}}; //
  // null-initialization kills 21
  gt[58] = {{"56", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[93] = {{"56", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"91", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[94] = {{"56", OpenSSLEVPKeyDerivationState::PARAM_INIT},
            {"21", OpenSSLEVPKeyDerivationState::PARAM_INIT},
            {"91", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  gt[100] = {{"56", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"21", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"91", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"98", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  gt[101] = {{"56", OpenSSLEVPKeyDerivationState::DERIVED},
             {"21", OpenSSLEVPKeyDerivationState::DERIVED},
             {"91", OpenSSLEVPKeyDerivationState::DERIVED},
             {"98", OpenSSLEVPKeyDerivationState::DERIVED}};
  gt[148] = {{"56", OpenSSLEVPKeyDerivationState::DERIVED},
             {"21", OpenSSLEVPKeyDerivationState::DERIVED},
             {"91", OpenSSLEVPKeyDerivationState::DERIVED},
             {"98", OpenSSLEVPKeyDerivationState::DERIVED},
             {"147", OpenSSLEVPKeyDerivationState::DERIVED}};
  gt[149] = {{"56", OpenSSLEVPKeyDerivationState::UNINIT},
             {"21", OpenSSLEVPKeyDerivationState::UNINIT},
             {"91", OpenSSLEVPKeyDerivationState::UNINIT},
             {"98", OpenSSLEVPKeyDerivationState::UNINIT},
             {"147", OpenSSLEVPKeyDerivationState::UNINIT}};
  compareResults(gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation4) {
  Initialize({pathToLLFiles + "key-derivation4_c.ll"});

  std::map<std::size_t, std::map<std::string, int>> gt;

  // gt[57] = {{"21", OpenSSLEVPKeyDerivationState::UNINIT}}; //
  // null-initialization kills 21
  gt[59] = {{"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"57", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[104] = {{"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"57", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"102", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[105] = {{"21", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"57", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"102", OpenSSLEVPKeyDerivationState::PARAM_INIT}};

  // TODO: Should FREE on PARAM_INIT result in UNINIT, or in ERROR? (currently
  // it is UNINIT)

  gt[152] = {{"21", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"57", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"102", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"151", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  gt[153] = {{"21", OpenSSLEVPKeyDerivationState::UNINIT},
             {"57", OpenSSLEVPKeyDerivationState::UNINIT},
             {"102", OpenSSLEVPKeyDerivationState::UNINIT},
             {"151", OpenSSLEVPKeyDerivationState::UNINIT}};
  compareResults(gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation5) {
  Initialize({pathToLLFiles + "key-derivation5_c.ll"});

  std::map<std::size_t, std::map<std::string, int>> gt;

  // gt[58] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT}};//
  // null-initialization kills 22
  gt[60] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[105] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"103", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[106] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  gt[112] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"110", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  gt[113] = gt[160] = {{"22", OpenSSLEVPKeyDerivationState::DERIVED},
                       {"58", OpenSSLEVPKeyDerivationState::DERIVED},
                       {"103", OpenSSLEVPKeyDerivationState::DERIVED},
                       {"110", OpenSSLEVPKeyDerivationState::DERIVED}};
  // should report an error at 160? (kdf_ctx is not freed)
  compareResults(gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, DISABLED_KeyDerivation6) {
  Initialize({pathToLLFiles + "key-derivation6_c.ll"});
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, int>> gt;
  gt[102] = {{"100", OpenSSLEVPKeyDerivationState::BOT},
             {"22", OpenSSLEVPKeyDerivationState::BOT}};
  gt[103] = {{"100", OpenSSLEVPKeyDerivationState::ERROR},
             {"22", OpenSSLEVPKeyDerivationState::ERROR}};
  gt[109] = gt[110] = {{"100", OpenSSLEVPKeyDerivationState::ERROR},
                       {"22", OpenSSLEVPKeyDerivationState::ERROR},
                       {"107", OpenSSLEVPKeyDerivationState::ERROR}};
  compareResults(gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation7) {
  Initialize({pathToLLFiles + "key-derivation7_c.ll"});
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, int>> gt;

  // gt[57] = {{"21", OpenSSLEVPKeyDerivationState::UNINIT}}; //
  // null-initialization kills 21
  gt[59] = {{"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"57", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[104] = {{"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"57", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"102", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[105] = {{"21", OpenSSLEVPKeyDerivationState::ERROR},
             {"57", OpenSSLEVPKeyDerivationState::ERROR},
             {"102", OpenSSLEVPKeyDerivationState::ERROR}};
  gt[152] = gt[153] = {{"21", OpenSSLEVPKeyDerivationState::ERROR},
                       {"57", OpenSSLEVPKeyDerivationState::ERROR},
                       {"102", OpenSSLEVPKeyDerivationState::ERROR},
                       {"151", OpenSSLEVPKeyDerivationState::ERROR}};
  compareResults(gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation8) {
  Initialize({pathToLLFiles + "key-derivation8_c.ll"});
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, int>> gt;

  // gt[58] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT}}; //
  // null-initialization kills 22
  gt[60] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[105] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"103", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  gt[107] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  gt[112] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"110", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  gt[113] = {{"22", OpenSSLEVPKeyDerivationState::DERIVED},
             {"58", OpenSSLEVPKeyDerivationState::DERIVED},
             {"103", OpenSSLEVPKeyDerivationState::DERIVED},
             {"110", OpenSSLEVPKeyDerivationState::DERIVED}};
  gt[160] = {{"22", OpenSSLEVPKeyDerivationState::DERIVED},
             {"58", OpenSSLEVPKeyDerivationState::DERIVED},
             {"103", OpenSSLEVPKeyDerivationState::DERIVED},
             {"110", OpenSSLEVPKeyDerivationState::DERIVED},
             {"159", OpenSSLEVPKeyDerivationState::DERIVED}};
  gt[161] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT},
             {"58", OpenSSLEVPKeyDerivationState::UNINIT},
             {"103", OpenSSLEVPKeyDerivationState::UNINIT},
             {"110", OpenSSLEVPKeyDerivationState::UNINIT},
             {"159", OpenSSLEVPKeyDerivationState::UNINIT}};
  compareResults(gt);
}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
