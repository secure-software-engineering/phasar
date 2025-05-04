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
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureMemoryDescription.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <memory>
#include <optional>

using namespace std;
using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETSAnalysisOpenSSLSecureMemoryTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("openssl/secure_memory/");
  const std::vector<std::string> EntryPoints = {"main"};

  std::optional<HelperAnalyses> HA;
  OpenSSLSecureMemoryDescription Desc{};
  std::optional<IDETypeStateAnalysis<OpenSSLSecureMemoryDescription>> TSProblem;
  unique_ptr<IDESolver_P<IDETypeStateAnalysis<OpenSSLSecureMemoryDescription>>>
      Llvmtssolver;

  enum OpenSSLSecureMemoryState {
    TOP = 42,
    BOT = 0,
    ZEROED = 1,
    FREED = 2,
    ERROR = 3,
    ALLOCATED = 4
  };
  IDETSAnalysisOpenSSLSecureMemoryTest() = default;
  ~IDETSAnalysisOpenSSLSecureMemoryTest() override = default;

  void initialize(const llvm::Twine &&IRFile) {
    HA.emplace(IRFile, EntryPoints);

    TSProblem = createAnalysisProblem<
        IDETypeStateAnalysis<OpenSSLSecureMemoryDescription>>(*HA, &Desc,
                                                              EntryPoints);
    Llvmtssolver = make_unique<
        IDESolver_P<IDETypeStateAnalysis<OpenSSLSecureMemoryDescription>>>(
        *TSProblem, &HA->getICFG());

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
              getMetaDataID(Result.first), int(Result.second)));
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

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory1) {
  initialize({PathToLlFiles + "memory1_c.ll"});
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, int>> Gt;
  // TODO add GT values
  Gt[10] = {{"8", OpenSSLSecureMemoryState::ALLOCATED},
            {"3", OpenSSLSecureMemoryState::ALLOCATED}};
  Gt[20] = Gt[10];
  Gt[29] = {{"3", OpenSSLSecureMemoryState::ERROR}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory2) {
  initialize({PathToLlFiles + "memory2_c.ll"});
  std::map<size_t, std::map<std::string, int>> Gt;
  Gt[10] = {{"8", OpenSSLSecureMemoryState::ALLOCATED},
            {"3", OpenSSLSecureMemoryState::ALLOCATED}};
  Gt[20] = Gt[10];
  Gt[30] = {{"8", OpenSSLSecureMemoryState::ZEROED},
            {"3", OpenSSLSecureMemoryState::ZEROED},
            {"27", OpenSSLSecureMemoryState::ZEROED}};
  Gt[32] = {{"8", OpenSSLSecureMemoryState::FREED},
            {"3", OpenSSLSecureMemoryState::FREED},
            {"27", OpenSSLSecureMemoryState::FREED},
            {"30", OpenSSLSecureMemoryState::FREED}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory3) {
  initialize({PathToLlFiles + "memory3_c.ll"});
  // llvmtssolver->printReport();
  std::map<size_t, std::map<std::string, int>> Gt;
  Gt[15] = {{"13", OpenSSLSecureMemoryState::ZEROED},
            {"6", OpenSSLSecureMemoryState::ZEROED}};

  // Imprecision of the analysis => write into buffer kills it permanently
  // instead of degrading the typestate

  // gt[34] = {{"6", OpenSSLSecureMemoryState::ALLOCATED}};
  // gt[49] = {{"6", OpenSSLSecureMemoryState::ALLOCATED},
  //          {"48", OpenSSLSecureMemoryState::ALLOCATED}};
  // gt[50] = {{"6", OpenSSLSecureMemoryState::ERROR},
  //          {"48", OpenSSLSecureMemoryState::ERROR}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory4) {
  initialize({PathToLlFiles + "memory4_c.ll"});
  std::map<size_t, std::map<std::string, int>> Gt;
  Gt[15] = {{"13", OpenSSLSecureMemoryState::ZEROED},
            {"6", OpenSSLSecureMemoryState::ZEROED}};

  // Imprecision of the analysis => write into buffer kills it permanently
  // instead of degrading the typestate (as for Memory3)

  // gt[34] = {{"6", OpenSSLSecureMemoryState::ALLOCATED}};
  // gt[49] = {{"6", OpenSSLSecureMemoryState::ZEROED},
  //           {"48", OpenSSLSecureMemoryState::ZEROED}};
  // gt[50] = {{"6", OpenSSLSecureMemoryState::FREED},
  //           {"48", OpenSSLSecureMemoryState::FREED}};
  compareResults(Gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory5) {
  initialize({PathToLlFiles + "memory5_c.ll"});
  std::map<size_t, std::map<std::string, int>> Gt;
  Gt[10] = {{"8", OpenSSLSecureMemoryState::ALLOCATED},
            {"3", OpenSSLSecureMemoryState::ALLOCATED}};

  // Imprecision of the analysis => write into buffer kills it permanently
  // instead of degrading the typestate (as for Memory3)
  // For whatever reason it nevertheless works

  Gt[22] = {{"3", OpenSSLSecureMemoryState::ALLOCATED}};
  Gt[31] = {{"3", OpenSSLSecureMemoryState::ALLOCATED},
            {"30", OpenSSLSecureMemoryState::ALLOCATED}};
  Gt[32] = {{"3", OpenSSLSecureMemoryState::ERROR},
            {"30", OpenSSLSecureMemoryState::ERROR}};
  compareResults(Gt);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
