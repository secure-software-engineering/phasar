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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPMDCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEVarTAOpenSSLMDTest : public ::testing::Test {
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
    OpenSSLEVPMDCTXDescription Desc(&staticRenaming);
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

TEST_F(IDEVarTAOpenSSLMDTest, Hash01) {
  TSAVarResults_t GroundTruth;
  GroundTruth[46]["45"] = {{"true", ALLOCATED}}; // EVP_CTX_new

  // GroundTruth[50]["45"] = {{"true", INITIALIZED}};
  // GroundTruth[50]["47"] = {{"true", INITIALIZED}};

  // GroundTruth[55]["45"] = {{"true", INITIALIZED}};
  // GroundTruth[55]["47"] = {{"true", INITIALIZED}};
  // GroundTruth[55]["53"] = {{"true", INITIALIZED}};

  // GroundTruth[59]["45"] = {{"true", FINALIZED}};
  // GroundTruth[59]["47"] = {{"true", FINALIZED}};
  // GroundTruth[59]["53"] = {{"true", FINALIZED}};
  // GroundTruth[59]["57"] = {{"true", FINALIZED}};

  GroundTruth[62]["45"] = {{"true", FREED}}; // ret
  GroundTruth[62]["47"] = {{"true", FREED}};
  GroundTruth[62]["53"] = {{"true", FREED}};
  GroundTruth[62]["57"] = {{"true", FREED}};

  doAnalysisAndCompareResults("hash01_c_dbg_xtc.ll", GroundTruth, false);
}

TEST_F(IDEVarTAOpenSSLMDTest, Hash02) {
  TSAVarResults_t GroundTruth;
  GroundTruth[46]["45"] = {{"true", ALLOCATED}};

  // GroundTruth[52]["45"] = {{"true", ERROR}};
  // GroundTruth[52]["50"] = {{"true", ERROR}};

  // GroundTruth[56]["45"] = {{"true", ERROR}};
  // GroundTruth[56]["50"] = {{"true", ERROR}};
  // GroundTruth[56]["54"] = {{"true", ERROR}};

  GroundTruth[59]["45"] = {{"true", ERROR}};
  GroundTruth[59]["50"] = {{"true", ERROR}};
  GroundTruth[59]["54"] = {{"true", ERROR}};
  GroundTruth[59]["57"] = {{"true", ERROR}};

  doAnalysisAndCompareResults("hash02_c_dbg_xtc.ll", GroundTruth, false);
}

TEST_F(IDEVarTAOpenSSLMDTest, DISABLED_Hash03) {
  TSAVarResults_t GroundTruth;

  GroundTruth[61]["60"] = {{"true", ALLOCATED}};
  GroundTruth[68]["60"] = {{"defined __static_condition11", INITIALIZED},
                           {"not defined __static_condition11", ALLOCATED}};

  // TODO: more GT

  doAnalysisAndCompareResults("hash03_c_dbg_xtc.ll", GroundTruth, true);
}

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
