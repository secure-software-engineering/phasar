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
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDETSAnalysisOpenSSLSecureHeapTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/openssl/secure_heap/";
  const std::set<std::string> EntryPoints = {"main"};

  ProjectIRDB *IRDB;
  LLVMTypeHierarchy *TH;
  LLVMBasedICFG *ICFG;
  LLVMPointsToInfo *PT;
  OpenSSLSecureHeapDescription *desc;
  IDETypeStateAnalysis *TSProblem;
  IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
            IDETypeStateAnalysis::f_t, IDETypeStateAnalysis::t_t,
            IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
            IDETypeStateAnalysis::i_t> *llvmtssolver = 0;
  IDESolver<const llvm::Instruction *, SecureHeapFact, const llvm::Function *,
            const llvm::StructType *, const llvm::Value *, SecureHeapValue,
            LLVMBasedICFG> *secureHeapPropagationResults;
  IDESecureHeapPropagation *secureHeapPropagationProblem;
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
  virtual ~IDETSAnalysisOpenSSLSecureHeapTest() = default;

  void Initialize(const std::vector<std::string> &IRFiles) {
    IRDB = new ProjectIRDB(IRFiles, IRDBOptions::WPA);
    TH = new LLVMTypeHierarchy(*IRDB);
    PT = new LLVMPointsToInfo(*IRDB);
    ICFG = new LLVMBasedICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, TH,
                             PT);

    secureHeapPropagationProblem =
        new IDESecureHeapPropagation(IRDB, TH, ICFG, PT, EntryPoints);
    secureHeapPropagationResults =
        new IDESolver<const llvm::Instruction *, SecureHeapFact,
                      const llvm::Function *, const llvm::StructType *,
                      const llvm::Value *, SecureHeapValue, LLVMBasedICFG>(
            *secureHeapPropagationProblem);

    desc = new OpenSSLSecureHeapDescription(*secureHeapPropagationResults);
    TSProblem =
        new IDETypeStateAnalysis(IRDB, TH, ICFG, PT, *desc, EntryPoints);
    llvmtssolver =
        new IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
                      IDETypeStateAnalysis::f_t, IDETypeStateAnalysis::t_t,
                      IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
                      IDETypeStateAnalysis::i_t>(*TSProblem);

    secureHeapPropagationResults->solve();
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

TEST_F(IDETSAnalysisOpenSSLSecureHeapTest, Memory6) {
  Initialize({pathToLLFiles + "memory6_c.ll"});

  // secureHeapPropagationResults->printReport();

  std::map<std::size_t, std::map<std::string, int>> gt;
  gt[25] = {{"9", OpenSSLSecureHeapState::ZEROED},
            {"23", OpenSSLSecureHeapState::ZEROED}};

  // the analysis ignores strcpy, so we are getting FREED instead of ERROR
  gt[31] = {{"9", OpenSSLSecureHeapState::FREED},
            {"23", OpenSSLSecureHeapState::FREED},
            {"29", OpenSSLSecureHeapState::FREED}};
  compareResults(gt);
}

TEST_F(IDETSAnalysisOpenSSLSecureHeapTest, Memory7) {
  Initialize({pathToLLFiles + "memory7_c.ll"});

  // secureHeapPropagationResults->printReport();

  std::map<std::size_t, std::map<std::string, int>> gt;
  gt[25] = {{"9", OpenSSLSecureHeapState::ZEROED},
            {"23", OpenSSLSecureHeapState::ZEROED}};
  // here FREED is correct
  gt[32] = {{"9", OpenSSLSecureHeapState::FREED},
            {"23", OpenSSLSecureHeapState::FREED},
            {"29", OpenSSLSecureHeapState::FREED}};
  compareResults(gt);
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
