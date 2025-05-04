/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFDescription.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <memory>

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETSAnalysisOpenSSLEVPKDFTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("openssl/key_derivation/");
  const std::vector<std::string> EntryPoints = {"main"};

  std::optional<HelperAnalyses> HA;
  std::optional<OpenSSLEVPKDFCTXDescription> OpenSSLEVPKeyDerivationDesc;
  OpenSSLEVPKDFDescription OpenSSLEVPKDFDesc{};
  std::optional<IDETypeStateAnalysis<OpenSSLEVPKDFCTXDescription>> TSProblem;
  std::optional<IDETypeStateAnalysis<OpenSSLEVPKDFDescription>> TSKDFProblem;
  unique_ptr<IDESolver<IDETypeStateAnalysisDomain<OpenSSLEVPKDFCTXDescription>>>
      Llvmtssolver;
  unique_ptr<IDESolver<IDETypeStateAnalysisDomain<OpenSSLEVPKDFDescription>>>
      KdfSolver;

  // enum OpenSSLEVPKDFCTXState {
  //   TOP = 42,
  //   UNINIT = 5,
  //   CTX_ATTACHED = 1,
  //   PARAM_INIT = 2,
  //   DERIVED = 3,
  //   ERROR = 4,
  //   BOT = 0
  // };
  IDETSAnalysisOpenSSLEVPKDFTest() = default;
  ~IDETSAnalysisOpenSSLEVPKDFTest() override = default;

  void initialize(const llvm::Twine &IRFile) {
    HA.emplace(PathToLlFiles + IRFile, EntryPoints);

    TSKDFProblem =
        createAnalysisProblem<IDETypeStateAnalysis<OpenSSLEVPKDFDescription>>(
            *HA, &OpenSSLEVPKDFDesc, EntryPoints);

    KdfSolver = make_unique<
        IDESolver<IDETypeStateAnalysisDomain<OpenSSLEVPKDFDescription>>>(
        *TSKDFProblem, &HA->getICFG());

    OpenSSLEVPKeyDerivationDesc.emplace(*KdfSolver);

    TSProblem = createAnalysisProblem<
        IDETypeStateAnalysis<OpenSSLEVPKDFCTXDescription>>(
        *HA, &*OpenSSLEVPKeyDerivationDesc, EntryPoints);

    Llvmtssolver = make_unique<
        IDESolver<IDETypeStateAnalysisDomain<OpenSSLEVPKDFCTXDescription>>>(
        *TSProblem, &HA->getICFG());
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
      const std::map<std::size_t, std::map<std::string, OpenSSLEVPKDFCTXState>>
          &GroundTruth) {
    for (const auto &InstToGroundTruth : GroundTruth) {
      const auto *Inst =
          HA->getProjectIRDB().getInstruction(InstToGroundTruth.first);
      auto GT = InstToGroundTruth.second;
      std::map<std::string, OpenSSLEVPKDFCTXState> Results;
      for (auto Result : Llvmtssolver->resultsAt(Inst, true)) {
        if (Result.second != OpenSSLEVPKDFCTXState::BOT &&
            GT.count(getMetaDataID(Result.first))) {
          Results.insert(std::pair<std::string, OpenSSLEVPKDFCTXState>(
              getMetaDataID(Result.first), Result.second));
        }
      }
      EXPECT_EQ(Results, GT) << " at " << llvmIRToShortString(Inst);
    }
  }
}; // Test Fixture

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation1) {
  initialize("key-derivation1_c.ll");

  // llvmtssolver->printReport();

  std::map<std::size_t, std::map<std::string, OpenSSLEVPKDFCTXState>> Gt;

  Gt[48] = {{"46", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
            {"20", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[50] = {{"46", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
            {"20", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};

  Gt[92] = {{"46", OpenSSLEVPKDFCTXState::PARAM_INIT},
            {"20", OpenSSLEVPKDFCTXState::PARAM_INIT},
            {"88", OpenSSLEVPKDFCTXState::PARAM_INIT}};
  Gt[98] = {{"95", OpenSSLEVPKDFCTXState::DERIVED},
            {"46", OpenSSLEVPKDFCTXState::DERIVED},
            {"20", OpenSSLEVPKDFCTXState::DERIVED},
            {"88", OpenSSLEVPKDFCTXState::DERIVED}};
  Gt[146] = {{"144", OpenSSLEVPKDFCTXState::UNINIT},
             {"95", OpenSSLEVPKDFCTXState::UNINIT},
             {"46", OpenSSLEVPKDFCTXState::UNINIT},
             {"20", OpenSSLEVPKDFCTXState::UNINIT},
             {"88", OpenSSLEVPKDFCTXState::UNINIT}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation2) {
  initialize("key-derivation2_c.ll");

  std::map<std::size_t, std::map<std::string, OpenSSLEVPKDFCTXState>> Gt;
  // gt[40] = {{"22", OpenSSLEVPKDFCTXState::UNINIT}}; // killed by
  // null-initialization
  // gt[57] = {{"22", OpenSSLEVPKDFCTXState::UNINIT}}; // killed by
  // null-initialization
  Gt[60] = {{"22", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
            {"58", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[105] = {{"22", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
             {"58", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
             {"103", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[106] = {{"22", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"58", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"103", OpenSSLEVPKDFCTXState::PARAM_INIT}};
  Gt[112] = {{"22", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"58", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"103", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"110", OpenSSLEVPKDFCTXState::PARAM_INIT}};
  Gt[113] = {{"22", OpenSSLEVPKDFCTXState::DERIVED},
             {"58", OpenSSLEVPKDFCTXState::DERIVED},
             {"103", OpenSSLEVPKDFCTXState::DERIVED},
             {"110", OpenSSLEVPKDFCTXState::DERIVED}};
  Gt[160] = {{"22", OpenSSLEVPKDFCTXState::DERIVED},
             {"58", OpenSSLEVPKDFCTXState::DERIVED},
             {"103", OpenSSLEVPKDFCTXState::DERIVED},
             {"110", OpenSSLEVPKDFCTXState::DERIVED},
             {"159", OpenSSLEVPKDFCTXState::DERIVED}};
  Gt[161] = {{"22", OpenSSLEVPKDFCTXState::UNINIT},
             {"58", OpenSSLEVPKDFCTXState::UNINIT},
             {"103", OpenSSLEVPKDFCTXState::UNINIT},
             {"110", OpenSSLEVPKDFCTXState::UNINIT},
             {"159", OpenSSLEVPKDFCTXState::UNINIT}};
  // Fails due to merge conflicts: ID43 and ID162 have both value UNINIT on 22,
  // but it is implicit at ID43, so merging gives BOT

  // gt[164] = {{"22", OpenSSLEVPKDFCTXState::UNINIT}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation3) {
  initialize("key-derivation3_c.ll");
  std::map<std::size_t, std::map<std::string, OpenSSLEVPKDFCTXState>> Gt;

  // gt[56] = {{"21", OpenSSLEVPKDFCTXState::UNINIT}}; //
  // null-initialization kills 21
  Gt[58] = {{"56", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
            {"21", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[93] = {{"56", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
            {"21", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
            {"91", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[94] = {{"56", OpenSSLEVPKDFCTXState::PARAM_INIT},
            {"21", OpenSSLEVPKDFCTXState::PARAM_INIT},
            {"91", OpenSSLEVPKDFCTXState::PARAM_INIT}};
  Gt[100] = {{"56", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"21", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"91", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"98", OpenSSLEVPKDFCTXState::PARAM_INIT}};
  Gt[101] = {{"56", OpenSSLEVPKDFCTXState::DERIVED},
             {"21", OpenSSLEVPKDFCTXState::DERIVED},
             {"91", OpenSSLEVPKDFCTXState::DERIVED},
             {"98", OpenSSLEVPKDFCTXState::DERIVED}};
  Gt[148] = {{"56", OpenSSLEVPKDFCTXState::DERIVED},
             {"21", OpenSSLEVPKDFCTXState::DERIVED},
             {"91", OpenSSLEVPKDFCTXState::DERIVED},
             {"98", OpenSSLEVPKDFCTXState::DERIVED},
             {"147", OpenSSLEVPKDFCTXState::DERIVED}};
  Gt[149] = {{"56", OpenSSLEVPKDFCTXState::UNINIT},
             {"21", OpenSSLEVPKDFCTXState::UNINIT},
             {"91", OpenSSLEVPKDFCTXState::UNINIT},
             {"98", OpenSSLEVPKDFCTXState::UNINIT},
             {"147", OpenSSLEVPKDFCTXState::UNINIT}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation4) {
  initialize("key-derivation4_c.ll");

  std::map<std::size_t, std::map<std::string, OpenSSLEVPKDFCTXState>> Gt;

  // gt[57] = {{"21", OpenSSLEVPKDFCTXState::UNINIT}}; //
  // null-initialization kills 21
  Gt[59] = {{"21", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
            {"57", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[104] = {{"21", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
             {"57", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
             {"102", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[105] = {{"21", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"57", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"102", OpenSSLEVPKDFCTXState::PARAM_INIT}};

  // TODO: Should FREE on PARAM_INIT result in UNINIT, or in ERROR? (currently
  // it is UNINIT)

  Gt[152] = {{"21", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"57", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"102", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"151", OpenSSLEVPKDFCTXState::PARAM_INIT}};
  Gt[153] = {{"21", OpenSSLEVPKDFCTXState::UNINIT},
             {"57", OpenSSLEVPKDFCTXState::UNINIT},
             {"102", OpenSSLEVPKDFCTXState::UNINIT},
             {"151", OpenSSLEVPKDFCTXState::UNINIT}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation5) {
  initialize("key-derivation5_c.ll");

  std::map<std::size_t, std::map<std::string, OpenSSLEVPKDFCTXState>> Gt;

  // gt[58] = {{"22", OpenSSLEVPKDFCTXState::UNINIT}};//
  // null-initialization kills 22
  Gt[60] = {{"22", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
            {"58", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[105] = {{"22", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
             {"58", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
             {"103", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[106] = {{"22", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"58", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"103", OpenSSLEVPKDFCTXState::PARAM_INIT}};
  Gt[112] = {{"22", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"58", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"103", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"110", OpenSSLEVPKDFCTXState::PARAM_INIT}};
  Gt[113] = Gt[160] = {{"22", OpenSSLEVPKDFCTXState::DERIVED},
                       {"58", OpenSSLEVPKDFCTXState::DERIVED},
                       {"103", OpenSSLEVPKDFCTXState::DERIVED},
                       {"110", OpenSSLEVPKDFCTXState::DERIVED}};
  // should report an error at 160? (kdf_ctx is not freed)
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, DISABLED_KeyDerivation6) {
  initialize("key-derivation6_c.ll");
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, OpenSSLEVPKDFCTXState>> Gt;
  Gt[102] = {{"100", OpenSSLEVPKDFCTXState::BOT},
             {"22", OpenSSLEVPKDFCTXState::BOT}};
  Gt[103] = {{"100", OpenSSLEVPKDFCTXState::ERROR},
             {"22", OpenSSLEVPKDFCTXState::ERROR}};
  Gt[109] = Gt[110] = {{"100", OpenSSLEVPKDFCTXState::ERROR},
                       {"22", OpenSSLEVPKDFCTXState::ERROR},
                       {"107", OpenSSLEVPKDFCTXState::ERROR}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation7) {
  initialize("key-derivation7_c.ll");
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, OpenSSLEVPKDFCTXState>> Gt;

  // gt[57] = {{"21", OpenSSLEVPKDFCTXState::UNINIT}}; //
  // null-initialization kills 21
  Gt[59] = {{"21", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
            {"57", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[104] = {{"21", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
             {"57", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
             {"102", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[105] = {{"21", OpenSSLEVPKDFCTXState::ERROR},
             {"57", OpenSSLEVPKDFCTXState::ERROR},
             {"102", OpenSSLEVPKDFCTXState::ERROR}};
  Gt[152] = Gt[153] = {{"21", OpenSSLEVPKDFCTXState::ERROR},
                       {"57", OpenSSLEVPKDFCTXState::ERROR},
                       {"102", OpenSSLEVPKDFCTXState::ERROR},
                       {"151", OpenSSLEVPKDFCTXState::ERROR}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLEVPKDFTest, KeyDerivation8) {
  initialize("key-derivation8_c.ll");
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, OpenSSLEVPKDFCTXState>> Gt;

  // gt[58] = {{"22", OpenSSLEVPKDFCTXState::UNINIT}}; //
  // null-initialization kills 22
  Gt[60] = {{"22", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
            {"58", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[105] = {{"22", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
             {"58", OpenSSLEVPKDFCTXState::CTX_ATTACHED},
             {"103", OpenSSLEVPKDFCTXState::CTX_ATTACHED}};
  Gt[107] = {{"22", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"58", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"103", OpenSSLEVPKDFCTXState::PARAM_INIT}};
  Gt[112] = {{"22", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"58", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"103", OpenSSLEVPKDFCTXState::PARAM_INIT},
             {"110", OpenSSLEVPKDFCTXState::PARAM_INIT}};
  Gt[113] = {{"22", OpenSSLEVPKDFCTXState::DERIVED},
             {"58", OpenSSLEVPKDFCTXState::DERIVED},
             {"103", OpenSSLEVPKDFCTXState::DERIVED},
             {"110", OpenSSLEVPKDFCTXState::DERIVED}};
  Gt[160] = {{"22", OpenSSLEVPKDFCTXState::DERIVED},
             {"58", OpenSSLEVPKDFCTXState::DERIVED},
             {"103", OpenSSLEVPKDFCTXState::DERIVED},
             {"110", OpenSSLEVPKDFCTXState::DERIVED},
             {"159", OpenSSLEVPKDFCTXState::DERIVED}};
  Gt[161] = {{"22", OpenSSLEVPKDFCTXState::UNINIT},
             {"58", OpenSSLEVPKDFCTXState::UNINIT},
             {"103", OpenSSLEVPKDFCTXState::UNINIT},
             {"110", OpenSSLEVPKDFCTXState::UNINIT},
             {"159", OpenSSLEVPKDFCTXState::UNINIT}};
  compareResults(Gt);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
