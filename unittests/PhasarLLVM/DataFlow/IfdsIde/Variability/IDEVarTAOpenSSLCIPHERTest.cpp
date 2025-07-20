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
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPCIPHERCTXDescription.h"
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
class IDEVarTAOpenSSLCIPHERTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles =
      PHASAR_BUILD_SUBFOLDER("variability/Encryption/");
  const std::vector<std::string> EntryPoints = {"__main_9"};

  // inst ID => value ID => {Z3Constraint x typestate}
  using TSAVarResults_t = std::map<
      int,
      std::map<std::string,
               std::set<std::pair<std::string, OpenSSLEVPCIPHERCTXState>>>>;

  void doAnalysisAndCompareResults(const std::string &IRFilePath,
                                   TSAVarResults_t &GroundTruth,
                                   bool PrintDump = false) {
    LLVMProjectIRDB IRDB(PathToLLFiles + IRFilePath);
    if (PrintDump) {
      IRDB.emitPreprocessedIR(llvm::outs());
    }

    LLVMAliasSet PT(&IRDB);
    LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, EntryPoints, nullptr,
                       &PT,Soundness::Soundy, false);

    auto StaticRenaming = extractStaticRenaming(&IRDB);
    OpenSSLEVPCIPHERCTXDescription Desc(&StaticRenaming);
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
} // namespace

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
