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
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "gtest/gtest.h"

#include <memory>

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETSAnalysisOpenSSLEVPKDFTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/openssl/key_derivation/";
  const std::set<std::string> EntryPoints = {"main"};

  unique_ptr<ProjectIRDB> IRDB;
  unique_ptr<LLVMTypeHierarchy> TH;
  unique_ptr<LLVMBasedICFG> ICFG;
  unique_ptr<LLVMAliasInfo> PT;
  unique_ptr<OpenSSLEVPKDFCTXDescription> OpenSSLEVPKeyDerivationDesc;
  unique_ptr<OpenSSLEVPKDFDescription> OpenSSLEVPKDFDesc;
  unique_ptr<IDETypeStateAnalysis> TSProblem, TSKDFProblem;
  unique_ptr<IDESolver<IDETypeStateAnalysisDomain>> Llvmtssolver, KdfSolver;

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
  ~IDETSAnalysisOpenSSLEVPKDFTest() override = default;

  void initialize(const std::vector<std::string> &IRFiles) {
    IRDB = make_unique<ProjectIRDB>(IRFiles, IRDBOptions::WPA);
    TH = make_unique<LLVMTypeHierarchy>(*IRDB);
    PT = make_unique<LLVMAliasSet>(*IRDB);
    ICFG = make_unique<LLVMBasedICFG>(*IRDB, CallGraphAnalysisType::OTF,
                                      EntryPoints, TH.get(), PT.get());

    OpenSSLEVPKDFDesc = make_unique<OpenSSLEVPKDFDescription>();
    TSKDFProblem = make_unique<IDETypeStateAnalysis>(
        IRDB.get(), TH.get(), ICFG.get(), PT.get(), *OpenSSLEVPKDFDesc,
        EntryPoints);
    KdfSolver =
        make_unique<IDESolver<IDETypeStateAnalysisDomain>>(*TSKDFProblem);

    OpenSSLEVPKeyDerivationDesc =
        make_unique<OpenSSLEVPKDFCTXDescription>(*KdfSolver);
    TSProblem = make_unique<IDETypeStateAnalysis>(
        IRDB.get(), TH.get(), ICFG.get(), PT.get(),
        *OpenSSLEVPKeyDerivationDesc, EntryPoints);

    Llvmtssolver =
        make_unique<IDESolver<IDETypeStateAnalysisDomain>>(*TSProblem);
    KdfSolver->solve();
    Llvmtssolver->solve();
  }

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {}

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for
   argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  void compareResults(
      const std::map<std::size_t, std::map<std::string, int>> &GroundTruth) {
    for (const auto &InstToGroundTruth : GroundTruth) {
      auto *Inst = IRDB->getInstruction(InstToGroundTruth.first);
      auto GT = InstToGroundTruth.second;
      std::map<std::string, int> Results;
      for (auto Result : Llvmtssolver->resultsAt(Inst, true)) {
        if (Result.second != OpenSSLEVPKeyDerivationState::BOT &&
            GT.count(getMetaDataID(Result.first))) {
          Results.insert(std::pair<std::string, int>(
              getMetaDataID(Result.first), Result.second));
        }
      }
      EXPECT_EQ(Results, GT) << " at " << llvmIRToShortString(Inst);
    }
  }
}; // Test Fixture

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation1) {
  initialize({PathToLlFiles + "key-derivation1_c.ll"});

  // llvmtssolver->printReport();

  std::map<std::size_t, std::map<std::string, int>> Gt;

  Gt[48] = {{"46", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"20", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[50] = {{"46", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"20", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};

  Gt[92] = {{"46", OpenSSLEVPKeyDerivationState::PARAM_INIT},
            {"20", OpenSSLEVPKeyDerivationState::PARAM_INIT},
            {"88", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  Gt[98] = {{"95", OpenSSLEVPKeyDerivationState::DERIVED},
            {"46", OpenSSLEVPKeyDerivationState::DERIVED},
            {"20", OpenSSLEVPKeyDerivationState::DERIVED},
            {"88", OpenSSLEVPKeyDerivationState::DERIVED}};
  Gt[146] = {{"144", OpenSSLEVPKeyDerivationState::UNINIT},
             {"95", OpenSSLEVPKeyDerivationState::UNINIT},
             {"46", OpenSSLEVPKeyDerivationState::UNINIT},
             {"20", OpenSSLEVPKeyDerivationState::UNINIT},
             {"88", OpenSSLEVPKeyDerivationState::UNINIT}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation2) {
  initialize({PathToLlFiles + "key-derivation2_c.ll"});

  std::map<std::size_t, std::map<std::string, int>> Gt;
  // gt[40] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT}}; // killed by
  // null-initialization
  // gt[57] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT}}; // killed by
  // null-initialization
  Gt[60] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[105] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"103", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[106] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  Gt[112] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"110", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  Gt[113] = {{"22", OpenSSLEVPKeyDerivationState::DERIVED},
             {"58", OpenSSLEVPKeyDerivationState::DERIVED},
             {"103", OpenSSLEVPKeyDerivationState::DERIVED},
             {"110", OpenSSLEVPKeyDerivationState::DERIVED}};
  Gt[160] = {{"22", OpenSSLEVPKeyDerivationState::DERIVED},
             {"58", OpenSSLEVPKeyDerivationState::DERIVED},
             {"103", OpenSSLEVPKeyDerivationState::DERIVED},
             {"110", OpenSSLEVPKeyDerivationState::DERIVED},
             {"159", OpenSSLEVPKeyDerivationState::DERIVED}};
  Gt[161] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT},
             {"58", OpenSSLEVPKeyDerivationState::UNINIT},
             {"103", OpenSSLEVPKeyDerivationState::UNINIT},
             {"110", OpenSSLEVPKeyDerivationState::UNINIT},
             {"159", OpenSSLEVPKeyDerivationState::UNINIT}};
  // Fails due to merge conflicts: ID43 and ID162 have both value UNINIT on 22,
  // but it is implicit at ID43, so merging gives BOT

  // gt[164] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation3) {
  initialize({PathToLlFiles + "key-derivation3_c.ll"});
  std::map<std::size_t, std::map<std::string, int>> Gt;

  // gt[56] = {{"21", OpenSSLEVPKeyDerivationState::UNINIT}}; //
  // null-initialization kills 21
  Gt[58] = {{"56", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[93] = {{"56", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"91", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[94] = {{"56", OpenSSLEVPKeyDerivationState::PARAM_INIT},
            {"21", OpenSSLEVPKeyDerivationState::PARAM_INIT},
            {"91", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  Gt[100] = {{"56", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"21", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"91", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"98", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  Gt[101] = {{"56", OpenSSLEVPKeyDerivationState::DERIVED},
             {"21", OpenSSLEVPKeyDerivationState::DERIVED},
             {"91", OpenSSLEVPKeyDerivationState::DERIVED},
             {"98", OpenSSLEVPKeyDerivationState::DERIVED}};
  Gt[148] = {{"56", OpenSSLEVPKeyDerivationState::DERIVED},
             {"21", OpenSSLEVPKeyDerivationState::DERIVED},
             {"91", OpenSSLEVPKeyDerivationState::DERIVED},
             {"98", OpenSSLEVPKeyDerivationState::DERIVED},
             {"147", OpenSSLEVPKeyDerivationState::DERIVED}};
  Gt[149] = {{"56", OpenSSLEVPKeyDerivationState::UNINIT},
             {"21", OpenSSLEVPKeyDerivationState::UNINIT},
             {"91", OpenSSLEVPKeyDerivationState::UNINIT},
             {"98", OpenSSLEVPKeyDerivationState::UNINIT},
             {"147", OpenSSLEVPKeyDerivationState::UNINIT}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation4) {
  initialize({PathToLlFiles + "key-derivation4_c.ll"});

  std::map<std::size_t, std::map<std::string, int>> Gt;

  // gt[57] = {{"21", OpenSSLEVPKeyDerivationState::UNINIT}}; //
  // null-initialization kills 21
  Gt[59] = {{"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"57", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[104] = {{"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"57", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"102", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[105] = {{"21", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"57", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"102", OpenSSLEVPKeyDerivationState::PARAM_INIT}};

  // TODO: Should FREE on PARAM_INIT result in UNINIT, or in ERROR? (currently
  // it is UNINIT)

  Gt[152] = {{"21", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"57", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"102", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"151", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  Gt[153] = {{"21", OpenSSLEVPKeyDerivationState::UNINIT},
             {"57", OpenSSLEVPKeyDerivationState::UNINIT},
             {"102", OpenSSLEVPKeyDerivationState::UNINIT},
             {"151", OpenSSLEVPKeyDerivationState::UNINIT}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation5) {
  initialize({PathToLlFiles + "key-derivation5_c.ll"});

  std::map<std::size_t, std::map<std::string, int>> Gt;

  // gt[58] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT}};//
  // null-initialization kills 22
  Gt[60] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[105] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"103", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[106] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  Gt[112] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"110", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  Gt[113] = Gt[160] = {{"22", OpenSSLEVPKeyDerivationState::DERIVED},
                       {"58", OpenSSLEVPKeyDerivationState::DERIVED},
                       {"103", OpenSSLEVPKeyDerivationState::DERIVED},
                       {"110", OpenSSLEVPKeyDerivationState::DERIVED}};
  // should report an error at 160? (kdf_ctx is not freed)
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, DISABLED_KeyDerivation6) {
  initialize({PathToLlFiles + "key-derivation6_c.ll"});
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, int>> Gt;
  Gt[102] = {{"100", OpenSSLEVPKeyDerivationState::BOT},
             {"22", OpenSSLEVPKeyDerivationState::BOT}};
  Gt[103] = {{"100", OpenSSLEVPKeyDerivationState::ERROR},
             {"22", OpenSSLEVPKeyDerivationState::ERROR}};
  Gt[109] = Gt[110] = {{"100", OpenSSLEVPKeyDerivationState::ERROR},
                       {"22", OpenSSLEVPKeyDerivationState::ERROR},
                       {"107", OpenSSLEVPKeyDerivationState::ERROR}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation7) {
  initialize({PathToLlFiles + "key-derivation7_c.ll"});
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, int>> Gt;

  // gt[57] = {{"21", OpenSSLEVPKeyDerivationState::UNINIT}}; //
  // null-initialization kills 21
  Gt[59] = {{"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"57", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[104] = {{"21", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"57", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"102", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[105] = {{"21", OpenSSLEVPKeyDerivationState::ERROR},
             {"57", OpenSSLEVPKeyDerivationState::ERROR},
             {"102", OpenSSLEVPKeyDerivationState::ERROR}};
  Gt[152] = Gt[153] = {{"21", OpenSSLEVPKeyDerivationState::ERROR},
                       {"57", OpenSSLEVPKeyDerivationState::ERROR},
                       {"102", OpenSSLEVPKeyDerivationState::ERROR},
                       {"151", OpenSSLEVPKeyDerivationState::ERROR}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation8) {
  initialize({PathToLlFiles + "key-derivation8_c.ll"});
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, int>> Gt;

  // gt[58] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT}}; //
  // null-initialization kills 22
  Gt[60] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
            {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[105] = {{"22", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"58", OpenSSLEVPKeyDerivationState::CTX_ATTACHED},
             {"103", OpenSSLEVPKeyDerivationState::CTX_ATTACHED}};
  Gt[107] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  Gt[112] = {{"22", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"58", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"103", OpenSSLEVPKeyDerivationState::PARAM_INIT},
             {"110", OpenSSLEVPKeyDerivationState::PARAM_INIT}};
  Gt[113] = {{"22", OpenSSLEVPKeyDerivationState::DERIVED},
             {"58", OpenSSLEVPKeyDerivationState::DERIVED},
             {"103", OpenSSLEVPKeyDerivationState::DERIVED},
             {"110", OpenSSLEVPKeyDerivationState::DERIVED}};
  Gt[160] = {{"22", OpenSSLEVPKeyDerivationState::DERIVED},
             {"58", OpenSSLEVPKeyDerivationState::DERIVED},
             {"103", OpenSSLEVPKeyDerivationState::DERIVED},
             {"110", OpenSSLEVPKeyDerivationState::DERIVED},
             {"159", OpenSSLEVPKeyDerivationState::DERIVED}};
  Gt[161] = {{"22", OpenSSLEVPKeyDerivationState::UNINIT},
             {"58", OpenSSLEVPKeyDerivationState::UNINIT},
             {"103", OpenSSLEVPKeyDerivationState::UNINIT},
             {"110", OpenSSLEVPKeyDerivationState::UNINIT},
             {"159", OpenSSLEVPKeyDerivationState::UNINIT}};
  compareResults(Gt);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
