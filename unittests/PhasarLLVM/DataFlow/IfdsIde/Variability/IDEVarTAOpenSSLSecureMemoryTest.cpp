/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/IDEVarTabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureMemoryDescription.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"

#include "llvm/ADT/StringRef.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <limits>
#include <tuple>
#include <utility>

namespace {
using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEVarTabulationProblemTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles =
      PHASAR_BUILD_SUBFOLDER("variability/secure_memory/");
  const std::vector<std::string> EntryPoints = {"main"};

  // inst ID => value ID => {Z3Constraint x typestate}
  using TSAVarResults_t = std::map<
      int,
      std::map<std::string,
               std::set<std::pair<std::string, OpenSSLSecureMemoryState>>>>;

  void SetUp() override {
    // boost::log::core::get()->set_logging_enabled(false);
  }

  // IDELinearConstantAnalysis::lca_restults_t
  void doAnalysisAndCompareResults(const std::string &IRFilePath,
                                   TSAVarResults_t &GroundTruth,
                                   bool PrintDump = false) {
    LLVMProjectIRDB IRDB(PathToLLFiles + IRFilePath);
    if (PrintDump) {
      IRDB.emitPreprocessedIR(llvm::outs());
    }

    LLVMAliasSet PT(&IRDB);
    LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, EntryPoints, nullptr,
                       &PT, Soundness::Soundy, false);
    OpenSSLSecureMemoryDescription Desc;
    IDETypeStateAnalysis TSAProblem(&IRDB, &PT, &Desc, EntryPoints);

    IDEVarTabulationProblem VARAProblem(TSAProblem, ICFG);

    IDESolver TSASolver(&VARAProblem, &ICFG);

    TSASolver.solve();
    if (PrintDump) {
      TSASolver.dumpResults();
    }

    for (auto &[instId, Truth] : GroundTruth) {
      const auto *Inst = IRDB.getInstruction(instId);
      ASSERT_NE(nullptr, Inst);
      auto Results = TSASolver.resultsAt(Inst);

      EXPECT_EQ(Truth.size(), Results.size())
          << "No results at " << llvmIRToShortString(Inst);

      for (auto &[Fact, CondState] : Results) {
        auto FactId = getMetaDataID(Fact);
        bool Has = Truth.count(FactId);
        EXPECT_TRUE(Has);
        if (Has) {
          auto &TruthOfFact = Truth[FactId];
          EXPECT_EQ(TruthOfFact.size(), CondState.size());
          for (auto &[Cond, State] : CondState) {
            EXPECT_TRUE(TruthOfFact.count({to_string(Cond), State}));
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

}; // Test Fixture

static constexpr auto ALLOCATED = OpenSSLSecureMemoryState::ALLOCATED;

TEST_F(IDEVarTabulationProblemTest, DISABLED_HandleBasic_01) {
  TSAVarResults_t GroundTruth;
  GroundTruth[10]["9"] = {{"true", ALLOCATED}};
  GroundTruth[11]["9"] = {{"true", ALLOCATED}};
  GroundTruth[11]["4"] = {{"true", ALLOCATED}};

  doAnalysisAndCompareResults("memory1_1_c.ll", GroundTruth, true);
}

TEST_F(IDEVarTabulationProblemTest, HandleBasic_01_2) {
  TSAVarResults_t GroundTruth;
  // GroundTruth[10]["9"] = {{"true", ALLOCATED}};
  // GroundTruth[11]["9"] = {{"true", ALLOCATED}};
  // GroundTruth[11]["4"] = {{"true", ALLOCATED}};

  doAnalysisAndCompareResults("memory1_2_c.ll", GroundTruth, true);
}
} // namespace

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
