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
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDESecureHeapPropagation.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureHeapDescription.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/Twine.h"

#include "gtest/gtest.h"

#include <memory>
#include <optional>
#include <vector>

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETSAnalysisOpenSSLSecureHeapTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles = "llvm_test_code/openssl/secure_heap/";
  const std::vector<std::string> EntryPoints = {"main"};

  std::optional<HelperAnalyses> HA;

  std::optional<OpenSSLSecureHeapDescription> Desc;
  std::optional<IDETypeStateAnalysis> TSProblem;
  std::optional<IDESecureHeapPropagation> SecureHeapPropagationProblem;
  unique_ptr<IDESolver<IDETypeStateAnalysisDomain>> Llvmtssolver;
  unique_ptr<IDESolver<IDESecureHeapPropagationAnalysisDomain>>
      SecureHeapPropagationResults;
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

  void initialize(const std::string &IRFile) {
    HA.emplace(IRFile, EntryPoints);

    SecureHeapPropagationProblem =
        createAnalysisProblem<IDESecureHeapPropagation>(*HA, EntryPoints);
    SecureHeapPropagationResults =
        make_unique<IDESolver<IDESecureHeapPropagationAnalysisDomain>>(
            *SecureHeapPropagationProblem, &HA->getICFG());

    Desc.emplace(*SecureHeapPropagationResults);
    TSProblem =
        createAnalysisProblem<IDETypeStateAnalysis>(*HA, &*Desc, EntryPoints);
    Llvmtssolver = make_unique<IDESolver<IDETypeStateAnalysisDomain>>(
        *TSProblem, &HA->getICFG());

    SecureHeapPropagationResults->solve();
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
      auto *Inst = HA->getProjectIRDB().getInstruction(InstToGroundTruth.first);
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

TEST_F(IDETSAnalysisOpenSSLSecureHeapTest, DISABLED_Memory6) {
  initialize({PathToLlFiles + "memory6.ll"});

  // SecureHeapPropagationResults->dumpResults();

  std::map<std::size_t, std::map<std::string, int>> Gt;
  Gt[25] = {{"9", OpenSSLSecureHeapState::ZEROED},
            {"23", OpenSSLSecureHeapState::ZEROED}};

  // the analysis ignores strcpy, so we are getting FREED instead of ERROR
  Gt[31] = {{"9", OpenSSLSecureHeapState::FREED},
            {"23", OpenSSLSecureHeapState::FREED},
            {"29", OpenSSLSecureHeapState::FREED}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureHeapTest, DISABLED_Memory7) {
  initialize({PathToLlFiles + "memory7.ll"});

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
