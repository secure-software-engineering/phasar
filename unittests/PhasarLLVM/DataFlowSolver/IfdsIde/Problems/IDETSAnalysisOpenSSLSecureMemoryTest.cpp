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
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureMemoryDescription.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETSAnalysisOpenSSLSecureMemoryTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/openssl/secure_memory/";
  const std::set<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  LLVMPointsToInfo *PT;
  OpenSSLSecureMemoryDescription *desc;
  IDETypeStateAnalysis *TSProblem;
  IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
            IDETypeStateAnalysis::f_t, IDETypeStateAnalysis::t_t,
            IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
            IDETypeStateAnalysis::i_t> *llvmtssolver = 0;

  enum OpenSSLSecureMemoryState {
    TOP = 42,
    BOT = 0,
    ZEROED = 1,
    FREED = 2,
    ERROR = 3,
    ALLOCATED = 4
  };
  IDETSAnalysisOpenSSLSecureMemoryTest() = default;
  virtual ~IDETSAnalysisOpenSSLSecureMemoryTest() = default;

  void Initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles, IRDBOptions::WPA);
    TH = new LLVMTypeHierarchy(*IRDB);
    PT = new LLVMPointsToInfo(*IRDB);
    ICFG = new LLVMBasedICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, TH,
                             PT);
    desc = new OpenSSLSecureMemoryDescription();
    TSProblem =
        new IDETypeStateAnalysis(IRDB, TH, ICFG, PT, *desc, EntryPoints);
    llvmtssolver =
        new IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
                      IDETypeStateAnalysis::f_t, IDETypeStateAnalysis::t_t,
                      IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
                      IDETypeStateAnalysis::i_t>(*TSProblem);

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
    delete llvmtssolver;
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
        } // else {
        //   std::cout << "Unused result at " << InstToGroundTruth.first << ": "
        //             << llvmIRToShortString(Result.first) << " => "
        //             << Result.second << std::endl;
        // }
      }
      EXPECT_EQ(results, GT) << "at inst " << llvmIRToShortString(Inst);
    }
  }
}; // Test Fixture

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory1) {
  Initialize({pathToLLFiles + "memory1_c.ll"});
  // llvmtssolver->printReport();
  std::map<std::size_t, std::map<std::string, int>> gt;
  // TODO add GT values
  gt[10] = {{"8", OpenSSLSecureMemoryState::ALLOCATED},
            {"3", OpenSSLSecureMemoryState::ALLOCATED}};
  gt[20] = gt[10];
  gt[29] = {{"3", OpenSSLSecureMemoryState::ERROR}};
  compareResults(gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory2) {
  Initialize({pathToLLFiles + "memory2_c.ll"});
  std::map<size_t, std::map<std::string, int>> gt;
  gt[10] = {{"8", OpenSSLSecureMemoryState::ALLOCATED},
            {"3", OpenSSLSecureMemoryState::ALLOCATED}};
  gt[20] = gt[10];
  gt[30] = {{"8", OpenSSLSecureMemoryState::ZEROED},
            {"3", OpenSSLSecureMemoryState::ZEROED},
            {"27", OpenSSLSecureMemoryState::ZEROED}};
  gt[32] = {{"8", OpenSSLSecureMemoryState::FREED},
            {"3", OpenSSLSecureMemoryState::FREED},
            {"27", OpenSSLSecureMemoryState::FREED},
            {"30", OpenSSLSecureMemoryState::FREED}};
  compareResults(gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory3) {
  // boost::log::core::get()->set_logging_enabled(true);
  Initialize({pathToLLFiles + "memory3_c.ll"});
  // llvmtssolver->printReport();
  std::map<size_t, std::map<std::string, int>> gt;
  gt[15] = {{"13", OpenSSLSecureMemoryState::ZEROED},
            {"6", OpenSSLSecureMemoryState::ZEROED}};

  // Imprecision of the analysis => write into buffer kills it permanently
  // instead of degrading the typestate

  // gt[34] = {{"6", OpenSSLSecureMemoryState::ALLOCATED}};
  // gt[49] = {{"6", OpenSSLSecureMemoryState::ALLOCATED},
  //          {"48", OpenSSLSecureMemoryState::ALLOCATED}};
  // gt[50] = {{"6", OpenSSLSecureMemoryState::ERROR},
  //          {"48", OpenSSLSecureMemoryState::ERROR}};
  compareResults(gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory4) {
  Initialize({pathToLLFiles + "memory4_c.ll"});
  std::map<size_t, std::map<std::string, int>> gt;
  gt[15] = {{"13", OpenSSLSecureMemoryState::ZEROED},
            {"6", OpenSSLSecureMemoryState::ZEROED}};

  // Imprecision of the analysis => write into buffer kills it permanently
  // instead of degrading the typestate (as for Memory3)

  // gt[34] = {{"6", OpenSSLSecureMemoryState::ALLOCATED}};
  // gt[49] = {{"6", OpenSSLSecureMemoryState::ZEROED},
  //           {"48", OpenSSLSecureMemoryState::ZEROED}};
  // gt[50] = {{"6", OpenSSLSecureMemoryState::FREED},
  //           {"48", OpenSSLSecureMemoryState::FREED}};
  compareResults(gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureMemoryTest, Memory5) {
  Initialize({pathToLLFiles + "memory5_c.ll"});
  std::map<size_t, std::map<std::string, int>> gt;
  gt[10] = {{"8", OpenSSLSecureMemoryState::ALLOCATED},
            {"3", OpenSSLSecureMemoryState::ALLOCATED}};

  // Imprecision of the analysis => write into buffer kills it permanently
  // instead of degrading the typestate (as for Memory3)
  // For whatever reason it nevertheless works

  gt[22] = {{"3", OpenSSLSecureMemoryState::ALLOCATED}};
  gt[31] = {{"3", OpenSSLSecureMemoryState::ALLOCATED},
            {"30", OpenSSLSecureMemoryState::ALLOCATED}};
  gt[32] = {{"3", OpenSSLSecureMemoryState::ERROR},
            {"30", OpenSSLSecureMemoryState::ERROR}};
  compareResults(gt);
}

// main function for the test case
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
