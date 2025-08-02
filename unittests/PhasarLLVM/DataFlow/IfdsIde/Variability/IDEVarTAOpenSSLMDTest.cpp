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
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPMDCTXDescription.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/VarAlyzerExperiments/VarAlyzerUtils.h"
#include "phasar/Utils/Soundness.h"

#include "llvm/ADT/StringRef.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <limits>
#include <tuple>
#include <utility>

namespace {
using namespace psr;

#if __cplusplus >= 202002L
using enum OpenSSLEVPMDCTXState;
#else
static constexpr auto TOP = OpenSSLEVPMDCTXState::TOP;
static constexpr auto BOT = OpenSSLEVPMDCTXState::BOT;
static constexpr auto ALLOCATED = OpenSSLEVPMDCTXState::ALLOCATED;
static constexpr auto INITIALIZED = OpenSSLEVPMDCTXState::INITIALIZED;
static constexpr auto SIGN_INITIALIZED = OpenSSLEVPMDCTXState::SIGN_INITIALIZED;
static constexpr auto FINALIZED = OpenSSLEVPMDCTXState::FINALIZED;
static constexpr auto FREED = OpenSSLEVPMDCTXState::FREED;
static constexpr auto ERROR = OpenSSLEVPMDCTXState::ERROR;
static constexpr auto UNINIT = OpenSSLEVPMDCTXState::UNINIT;
#endif

/* ============== TEST FIXTURE ============== */
class IDEVarTAOpenSSLMDTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles =
      PHASAR_BUILD_SUBFOLDER("variability/hashing/");

  // inst ID => value ID => {Z3Constraint x typestate}
  using TSAVarResults_t = std::map<
      int, std::map<std::string,
                    std::set<std::pair<std::string, OpenSSLEVPMDCTXState>>>>;

  void doAnalysisAndCompareResults(const std::string &LLVMFilePath,
                                   llvm::ArrayRef<std::string> EntryPoints,
                                   TSAVarResults_t &GroundTruth,
                                   bool printDump = false) {
    LLVMProjectIRDB IRDB(PathToLLFiles + LLVMFilePath);
    if (printDump) {
      IRDB.emitPreprocessedIR(llvm::outs());
    }

    LLVMAliasSet PT(&IRDB);
    LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, EntryPoints, nullptr,
                       &PT, Soundness::Soundy, false);

    auto StaticRenaming = extractStaticRenaming(&IRDB);
    auto TnoI =
        extractDesugaredTypeNameOfInterest("EVP_MD_CTX", IRDB, StaticRenaming);
    ASSERT_TRUE(TnoI.has_value());

    OpenSSLEVPMDCTXDescription Desc(&StaticRenaming, *TnoI);
    IDETypeStateAnalysis TSAProblem(&IRDB, &PT, &Desc, EntryPoints);

    IDEVarTabulationProblem VARAProblem(TSAProblem, ICFG);

    IDESolver TSASolver(&VARAProblem, &ICFG);

    TSASolver.solve();
    if (printDump) {
      TSASolver.dumpResults();
    }

    for (auto &[instId, Truth] : GroundTruth) {
      const auto *Inst = IRDB.getInstruction(instId);
      ASSERT_NE(nullptr, Inst);
      auto Results = TSASolver.resultsAt(Inst);

      EXPECT_LE(Truth.size(), Results.size())
          << "No results at " << llvmIRToString(Inst);

      for (auto &[Fact, CondState] : Results) {
        auto FactId = getMetaDataID(Fact);
        bool has = Truth.count(FactId);
        // EXPECT_TRUE(has);
        if (has) {
          auto &TruthOfFact = Truth[FactId];
          EXPECT_LE(TruthOfFact.size(), CondState.size());

          for (auto &[Cond, State] : CondState) {
            EXPECT_TRUE(TruthOfFact.count({to_string(Cond), State}))
                << "Result (" << to_string(Cond) << ", "
                << Desc.stateToString(State) << ") not in GroundTruth["
                << instId << "]";
          }
        }
      }
    }
  }

}; // Test Fixture

TEST_F(IDEVarTAOpenSSLMDTest, Hash01) {
  TSAVarResults_t GroundTruth;
  GroundTruth[46]["45"] = {{"true", ALLOCATED}}; // EVP_CTX_new

  // GroundTruth[50]["45"] = {{"true", INITIALIZED}};// not in the alias set

  // TODO: not in the resultsSet???
  // GroundTruth[50]["47"] = {{"true", INITIALIZED}};
  // GroundTruth[50]["41"] = {{"true", INITIALIZED}};

  // GroundTruth[55]["45"] = {{"true", INITIALIZED}};// not in the alias set
  // GroundTruth[55]["47"] = {{"true", INITIALIZED}};// not in the alias set

  // TODO: not in the resultsSet???
  // GroundTruth[55]["53"] = {{"true", INITIALIZED}};
  // GroundTruth[55]["41"] = {{"true", INITIALIZED}};

  // GroundTruth[59]["45"] = {{"true", FINALIZED}};// not in the alias set
  // GroundTruth[59]["47"] = {{"true", FINALIZED}};// not in the alias set
  // GroundTruth[59]["53"] = {{"true", FINALIZED}};// not in the alias set

  // TODO: not in the resultsSet???
  // GroundTruth[59]["57"] = {{"true", FINALIZED}};
  // GroundTruth[59]["41"] = {{"true", FINALIZED}};

  // ret
  GroundTruth[61]["41"] = {{"true", FREED}}; // the alloca
  // GroundTruth[62]["45"] = {{"true", FREED}}; // not in the alias set
  // GroundTruth[62]["47"] = {{"true", FREED}}; // not in the alias set
  // GroundTruth[62]["53"] = {{"true", FREED}}; // not in the alias set
  // GroundTruth[62]["57"] = {{"true", FREED}}; // not in the alias set
  GroundTruth[61]["59"] = {
      {"true", FREED}}; // the load that gets directly passed to the free mthd

  doAnalysisAndCompareResults("hash01_xtc_c_dbg.ll", {"__main_21"}, GroundTruth,
                              true);
}

TEST_F(IDEVarTAOpenSSLMDTest, Hash02) {
  TSAVarResults_t GroundTruth;
  GroundTruth[46]["45"] = {{"true", ALLOCATED}};

  // TODO: No results??
  // GroundTruth[52]["45"] = {{"true", ERROR}};
  // GroundTruth[52]["50"] = {{"true", ERROR}};
  // GroundTruth[52]["41"] = {{"true", ERROR}};

  // TODO: No results??
  // GroundTruth[56]["45"] = {{"true", ERROR}};
  // GroundTruth[56]["50"] = {{"true", ERROR}};
  // GroundTruth[56]["54"] = {{"true", ERROR}};
  // GroundTruth[56]["41"] = {{"true", ERROR}};

  // GroundTruth[59]["45"] = {{"true", ERROR}}; // not in alias set
  GroundTruth[58]["50"] = {{"true", ERROR}};
  GroundTruth[58]["54"] = {{"true", ERROR}};
  GroundTruth[58]["56"] = {{"true", ERROR}};
  GroundTruth[58]["41"] = {{"true", ERROR}};

  doAnalysisAndCompareResults("hash02_xtc_c_dbg.ll", {"__main_21"}, GroundTruth,
                              true);
}

TEST_F(IDEVarTAOpenSSLMDTest, Hash03) {
  TSAVarResults_t GroundTruth;

  // TODO: Fix ground truth

  GroundTruth[61]["60"] = {{"true", ALLOCATED}};
  GroundTruth[73]["60"] = {{"|(defined A)|", INITIALIZED},
                           {"(not |(defined A)|)", ALLOCATED}};

  // TODO: more GT

  doAnalysisAndCompareResults("hash03_xtc_c_dbg.ll", {"__main_21"}, GroundTruth,
                              true);
}
} // namespace

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
