/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann, and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoFullConstantPropagation.h"
#include "phasar/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/CallString.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/IntraMonoSolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "gtest/gtest.h"

#include "llvm/Support/raw_ostream.h"

#include <set>
#include <string>
#include <tuple>

#include "TestConfig.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IntraMonoFullConstantPropagationTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles = unittest::PathToLLTestFiles;
  const std::set<std::string> EntryPoints = {"main"};

  using IMFCPCompactResult_t =
      std::tuple<std::string, std::size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>;
  LLVMProjectIRDB *IRDB = nullptr;

  void SetUp() override {}
  void TearDown() override { delete IRDB; }

  void
  doAnalysisAndCompareResults(llvm::StringRef LlvmFilePath,
                              const std::set<IMFCPCompactResult_t> &GroundTruth,
                              bool PrintDump = false) {
    IRDB = new LLVMProjectIRDB(PathToLlFiles + LlvmFilePath);
    if (PrintDump) {
      IRDB->dump();
    }
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToSet PT(*IRDB);
    LLVMBasedICFG ICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, &TH,
                       &PT);
    IntraMonoFullConstantPropagation FCP(IRDB, &TH, &ICFG, &PT, EntryPoints);
    IntraMonoSolver_P<IntraMonoFullConstantPropagation> IMSolver(FCP);
    IMSolver.solve();
    if (PrintDump) {
      IMSolver.dumpResults();
    }
    llvm::outs() << "Done analysis!\n";
    // do the comparison
    bool ResultNotEmpty = false;
    for (const auto &Truth : GroundTruth) {
      const auto *Fun = IRDB->getFunctionDefinition(std::get<0>(Truth));
      const auto *Line = getNthInstruction(Fun, std::get<1>(Truth));
      auto ResultSet = IMSolver.getResultsAt(Line);
      for (const auto &[Fact, Value] : ResultSet) {
        std::string FactStr = llvmIRToString(Fact);
        llvm::StringRef FactRef(FactStr);
        if (FactRef.startswith("%" + std::get<2>(Truth) + " ")) {
          llvm::outs() << "Checking variable: " << FactStr << '\n';
          ResultNotEmpty = true;
          EXPECT_EQ(std::get<3>(Truth), Value);
        }
      }
    }
    EXPECT_TRUE(ResultNotEmpty);
  }

}; // Test Fixture

// Test for Case I of Store
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_01) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 5, "i", 13));
  doAnalysisAndCompareResults("full_constant/basic_01_cpp.ll", GroundTruth,
                              true);
}

// Test for Case II of Store and Load Inst
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_02) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 8, "i", 13));
  doAnalysisAndCompareResults("full_constant/basic_02_cpp.ll", GroundTruth,
                              true);
}

// Test for Operators
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_03) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 9, "i", 13));
  doAnalysisAndCompareResults("full_constant/basic_03_cpp.ll", GroundTruth,
                              true);
}

// Test for Operators
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_04) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 11, "i", 13));
  doAnalysisAndCompareResults("full_constant/basic_04_cpp.ll", GroundTruth,
                              true);
}

// Test for Operators
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_05) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 8, "i", 13));
  doAnalysisAndCompareResults("full_constant/basic_05_cpp.ll", GroundTruth,
                              true);
}

// Test for Operators
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_06) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 7, "i", 9));
  doAnalysisAndCompareResults("full_constant/basic_06_cpp.ll", GroundTruth,
                              true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
