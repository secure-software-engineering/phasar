/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "gtest/gtest.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDESecureHeapPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureHeapDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETSAnalysisOpenSSLSecureHeapTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/openssl/secure_heap/";
  const std::set<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB{};
  LLVMTypeHierarchy *TH{};
  LLVMBasedICFG *ICFG{};
  LLVMPointsToInfo *PT{};
  OpenSSLSecureHeapDescription *Desc{};
  IDETypeStateAnalysis *TSProblem{};
  IDESolver<IDETypeStateAnalysisDomain> *Llvmtssolver{};
  IDESolver<IDESecureHeapPropagationAnalysisDomain>
      *SecureHeapPropagationResults{};
  IDESecureHeapPropagation *SecureHeapPropagationProblem{};
  enum OpenSSLSecureHeapState {
    TOP = 42,
    BOT = 0,
    UNINIT = 1,
    ALLOCATED = 2,
    ZEROED = 3,
    FREED = 4,
    ERROR = 5
  };
  IDETSAnalysisOpenSSLSecureHeapTest() = default;
  ~IDETSAnalysisOpenSSLSecureHeapTest() override = default;

  void initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles, IRDBOptions::WPA);
    TH = new LLVMTypeHierarchy(*IRDB);
    PT = new LLVMPointsToSet(*IRDB);
    ICFG = new LLVMBasedICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, TH,
                             PT);

    SecureHeapPropagationProblem =
        new IDESecureHeapPropagation(IRDB, TH, ICFG, PT, EntryPoints);
    SecureHeapPropagationResults =
        new IDESolver<IDESecureHeapPropagationAnalysisDomain>(
            *SecureHeapPropagationProblem);

    Desc = new OpenSSLSecureHeapDescription(*SecureHeapPropagationResults);
    TSProblem =
        new IDETypeStateAnalysis(IRDB, TH, ICFG, PT, *Desc, EntryPoints);
    Llvmtssolver = new IDESolver<IDETypeStateAnalysisDomain>(*TSProblem);

    SecureHeapPropagationResults->solve();
    Llvmtssolver->solve();
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
    delete Llvmtssolver;
  }

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
        if (GT.find(getMetaDataID(Result.first)) != GT.end()) {
          Results.insert(std::pair<std::string, int>(
              getMetaDataID(Result.first), Result.second));
        } // else {
        //   std::cout << "Unused result at " << InstToGroundTruth.first << ": "
        //             << llvmIRToShortString(Result.first) << " => "
        //             << Result.second << std::endl;
        // }
      }
      EXPECT_EQ(Results, GT) << "at inst " << llvmIRToShortString(Inst);
    }
  }
}; // Test Fixture

TEST_F(IDETSAnalysisOpenSSLSecureHeapTest, Memory6) {
  initialize({PathToLlFiles + "memory6_c.ll"});

  // secureHeapPropagationResults->printReport();

  std::map<std::size_t, std::map<std::string, int>> Gt;
  Gt[25] = {{"9", OpenSSLSecureHeapState::ZEROED},
            {"23", OpenSSLSecureHeapState::ZEROED}};

  // the analysis ignores strcpy, so we are getting FREED instead of ERROR
  Gt[31] = {{"9", OpenSSLSecureHeapState::FREED},
            {"23", OpenSSLSecureHeapState::FREED},
            {"29", OpenSSLSecureHeapState::FREED}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureHeapTest, Memory7) {
  initialize({PathToLlFiles + "memory7_c.ll"});

  // secureHeapPropagationResults->printReport();

  std::map<std::size_t, std::map<std::string, int>> Gt;
  Gt[25] = {{"9", OpenSSLSecureHeapState::ZEROED},
            {"23", OpenSSLSecureHeapState::ZEROED}};
  // here FREED is correct
  Gt[32] = {{"9", OpenSSLSecureHeapState::FREED},
            {"23", OpenSSLSecureHeapState::FREED},
            {"29", OpenSSLSecureHeapState::FREED}};
  compareResults(Gt);
}

int main(int Argc, char *Argv[]) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
