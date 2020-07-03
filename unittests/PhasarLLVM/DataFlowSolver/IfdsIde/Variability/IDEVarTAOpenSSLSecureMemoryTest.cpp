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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureMemoryDescription.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEVarTabulationProblemTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/variability/secure_memory/";
  const std::set<std::string> EntryPoints = {"main"};

  // inst ID => value ID => {Z3Constraint x typestate}
  using TSAVarResults_t = std::map<
      int, std::map<std::string,
                    std::set<std::pair<
                        std::string, OpenSSLSecureMemoryDescription::State>>>>;
  enum OpenSSLSecureMemoryState {
    TOP = 42,
    BOT = 0,
    ZEROED = 1,
    FREED = 2,
    ERROR = 3,
    ALLOCATED = 4
  };
  ProjectIRDB *IRDB = nullptr;

  void SetUp() override {
    // boost::log::core::get()->set_logging_enabled(false);
  }

  // IDELinearConstantAnalysis::lca_restults_t
  void doAnalysisAndCompareResults(const std::string &llvmFilePath,
                                   TSAVarResults_t &GroundTruth,
                                   bool printDump = false) {
    IRDB = new ProjectIRDB({pathToLLFiles + llvmFilePath}, IRDBOptions::WPA);
    if (printDump) {
      IRDB->emitPreprocessedIR(std::cout, false);
    }
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToSet PT(*IRDB);
    LLVMBasedVarICFG VICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, &TH,
                           &PT);
    OpenSSLSecureMemoryDescription Desc;
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

      EXPECT_EQ(Truth.size(), Results.size())
          << "No results at " << llvmIRToShortString(Inst);

      for (auto &[Fact, CondState] : Results) {
        auto FactId = getMetaDataID(Fact);
        bool has = Truth.count(FactId);
        EXPECT_TRUE(has);
        if (has) {
          auto &TruthOfFact = Truth[FactId];
          EXPECT_EQ(TruthOfFact.size(), CondState.size());
          for (auto &[Cond, State] : CondState) {
            EXPECT_TRUE(TruthOfFact.count({Cond.to_string(), State}));
          }
        }
      }
    }

    /*for (auto &Truth : GroundTruth) {
      auto Fun = IRDB->getFunctionDefinition(std::get<0>(Truth));
      auto Inst = getNthInstruction(Fun, std::get<1>(Truth));
      auto Results = TSASolver.resultsAt(Inst);
      for (auto &[Fact, Value] : Results) {
        if (llvm::StringRef(llvmIRToString(Fact))
                .startswith(std::get<2>(Truth))) {
          for (auto &[Constraint, IntegerValue] : Value) {
            bool Found = false;
            for (auto &[TrueConstaint, TrueIntegerValue] : std::get<3>(Truth)) {
              // std::cout << "Comparing: " << Constraint.to_string() << " and "
              // << TrueConstaint << '\n';
              if (Constraint.to_string() == TrueConstaint) {
                EXPECT_TRUE(IntegerValue == TrueIntegerValue);
                Found = true;
                break;
              }
            }
            if (!Found) {
              FAIL() << "Could not find constraint: '" << Constraint.to_string()
                     << "' in ground truth!";
            }
          }
        }
      }
    }*/
  }

  void TearDown() override { delete IRDB; }

}; // Test Fixture

TEST_F(IDEVarTabulationProblemTest, DISABLED_HandleBasic_01) {
  TSAVarResults_t GroundTruth;
  GroundTruth[10]["9"] = {{"true", ALLOCATED}};
  GroundTruth[11]["9"] = {{"true", ALLOCATED}};
  GroundTruth[11]["4"] = {{"true", ALLOCATED}};

  doAnalysisAndCompareResults("memory1_c.ll", GroundTruth, true);
}

TEST_F(IDEVarTabulationProblemTest, HandleBasic_01_2) {
  TSAVarResults_t GroundTruth;
  // GroundTruth[10]["9"] = {{"true", ALLOCATED}};
  // GroundTruth[11]["9"] = {{"true", ALLOCATED}};
  // GroundTruth[11]["4"] = {{"true", ALLOCATED}};

  doAnalysisAndCompareResults("memory1_2_c.ll", GroundTruth, true);
}

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
