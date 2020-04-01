/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann, and others
 *****************************************************************************/

#include <set>
#include <string>
#include <tuple>

#include "gtest/gtest.h"

#include "llvm/Support/raw_ostream.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/CallString.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/InterMonoSolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class InterMonoTaintAnalysisTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/full_constant/";
  const std::set<std::string> EntryPoints = {"main"};

  using IMFCPCompactResult_t =
      std::tuple<std::string, std::size_t, std::string,
                 LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>;
  ProjectIRDB *IRDB = nullptr;

  void SetUp() override {
    std::cout << "setup\n";
    boost::log::core::get()->set_logging_enabled(false);
  }
  void TearDown() override { delete IRDB; }

  void doAnalysisAndCompareResults(std::string llvmFilePath,
                                   std::set<IMFCPCompactResult_t> GroundTruth,
                                   bool printDump = false) {
    IRDB = new ProjectIRDB({pathToLLFiles + llvmFilePath}, IRDBOptions::WPA);
    if (printDump) {
      IRDB->emitPreprocessedIR(std::cout, false);
    }
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToInfo PT(*IRDB);
    LLVMBasedICFG ICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, &TH,
                       &PT);
    InterMonoFullConstantPropagation FCP(IRDB, &TH, &ICFG, &PT, EntryPoints);
    InterMonoSolver_P<InterMonoFullConstantPropagation, 3> IMSolver(FCP);
    IMSolver.solve();
    if (printDump) {
      IMSolver.dumpResults();
    }
    // do the comparison
    for (auto &Truth : GroundTruth) {
      auto Fun = IRDB->getFunctionDefinition(std::get<0>(Truth));
      auto Line = getNthInstruction(Fun, std::get<1>(Truth));
      auto ResultSet = IMSolver.getResultsAt(Line);
      for (auto &[Fact, Value] : ResultSet) {
        std::string FactStr = llvmIRToString(Fact);
        llvm::StringRef FactRef(FactStr);
        if (FactRef.startswith("%" + std::get<2>(Truth) + " ")) {
          std::cout << "Checking variable: " << FactStr << std::endl;
          EXPECT_EQ(std::get<3>(Truth), Value);
        }
      }
    }
  }

}; // Test Fixture

TEST_F(InterMonoTaintAnalysisTest, BasicTest_01) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  // TODO needs to be adjusted
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>(
          "main", 1, "i", 13));
  doAnalysisAndCompareResults("basic_01_cpp.ll", GroundTruth, true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
