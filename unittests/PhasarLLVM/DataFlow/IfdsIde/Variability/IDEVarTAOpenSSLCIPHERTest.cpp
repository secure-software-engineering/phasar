/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#include <limits>
#include <tuple>
#include <utility>

#include "gtest/gtest.h"

#include "llvm/ADT/StringRef.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDEVarTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPCIPHERCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEVarTAOpenSSLCIPHERTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/variability/hashing/";
  const std::set<std::string> EntryPoints = {"__main_9"};

  // inst ID => value ID => {Z3Constraint x typestate}
  using TSAVarResults_t = std::map<
      int,
      std::map<
          std::string,
          std::set<std::pair<std::string, OpenSSLEVPMDCTXDescription::State>>>>;
  enum OpenSSLEVPMDCTXState {
    TOP = 42,
    BOT = 0,
    ALLOCATED,
    INITIALIZED,
    SIGN_INITIALIZED,
    FINALIZED,
    FREED,
    ERROR,
    UNINIT,
  };
  ProjectIRDB *IRDB = nullptr;

  void SetUp() override { boost::log::core::get()->set_logging_enabled(false); }

  void doAnalysisAndCompareResults(const std::string &llvmFilePath,
                                   TSAVarResults_t &GroundTruth,
                                   bool printDump = false) {
    IRDB = new ProjectIRDB({pathToLLFiles + llvmFilePath}, IRDBOptions::WPA);
    /*if (printDump) {
       IRDB->emitPreprocessedIR(std::cout, false);
     }*/
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToSet PT(*IRDB);
    LLVMBasedVarICFG VICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, &TH,
                           &PT);

    auto staticRenaming = extractStaticRenaming(IRDB);
    OpenSSLEVPCIPHERCTXDescription Desc(&staticRenaming);
    IDETypeStateAnalysis TSAProblem(IRDB, &TH, &VICFG, &PT, Desc, EntryPoints);

    IDEVarTabulationProblem_P<IDETypeStateAnalysis> VARAProblem(TSAProblem,
                                                                VICFG);

    IDESolver_P<IDEVarTabulationProblem_P<IDETypeStateAnalysis>> TSASolver(
        VARAProblem);

    TSASolver.solve();
    if (printDump) {
      TSASolver.dumpResults();
    }

    for (auto &[instId, Truth] : GroundTruth) {
      auto Inst = IRDB->getInstruction(instId);
      ASSERT_NE(nullptr, Inst);
      auto Results = TSASolver.resultsAt(Inst);

      EXPECT_LE(Truth.size(), Results.size())
          << "No results at " << llvmIRToShortString(Inst);

      for (auto &[Fact, CondState] : Results) {
        auto FactId = getMetaDataID(Fact);
        bool has = Truth.count(FactId);
        // EXPECT_TRUE(has);
        if (has) {
          auto &TruthOfFact = Truth[FactId];
          EXPECT_LE(TruthOfFact.size(), CondState.size());
          for (auto &[Cond, State] : CondState) {
            EXPECT_TRUE(TruthOfFact.count({Cond.to_string(), State}));
          }
        }
      }
    }
  }

  void TearDown() override { delete IRDB; }

}; // Test Fixture

TEST_F(IDEVarTAOpenSSLCIPHERTest, DISABLED_Crypt01) {
  TSAVarResults_t GroundTruth;
  // TODO

  doAnalysisAndCompareResults("crypt01_c_dbg_xtc.ll", GroundTruth, false);
}

TEST_F(IDEVarTAOpenSSLCIPHERTest, DISABLED_Crypt02) {
  TSAVarResults_t GroundTruth;
  // TODO

  doAnalysisAndCompareResults("crypt02_c_dbg_xtc.ll", GroundTruth, false);
}

TEST_F(IDEVarTAOpenSSLCIPHERTest, DISABLED_Crypt03) {
  TSAVarResults_t GroundTruth;

  // TODO
  doAnalysisAndCompareResults("crypt03_c_dbg_xtc.ll", GroundTruth, true);
}

TEST_F(IDEVarTAOpenSSLCIPHERTest, DISABLED_Crypt04) {
  TSAVarResults_t GroundTruth;

  // TODO
  doAnalysisAndCompareResults("crypt04_c_dbg_xtc.ll", GroundTruth, true);
}

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
