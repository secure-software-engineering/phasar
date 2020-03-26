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

#include <gtest/gtest.h>

#include <llvm/Support/raw_ostream.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/CallString.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoFullConstantPropagation.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/InterMonoSolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/BitVectorSet.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class InterMonoFullConstantPropagationTest : public ::testing::Test {
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
    std::cout << "Done analysis!\n";
    // do the comparison
    bool ResultNotEmpty = false;
    for (auto &Truth : GroundTruth) {
      auto Fun = IRDB->getFunctionDefinition(std::get<0>(Truth));
      auto Line = getNthInstruction(Fun, std::get<1>(Truth));
      auto ResultSet = IMSolver.getResultsAt(Line);
      for (auto &[Fact, Value] : ResultSet.getAsSet()) {
        std::string FactStr = llvmIRToString(Fact);
        llvm::StringRef FactRef(FactStr);
        if (FactRef.startswith("%" + std::get<2>(Truth) + " ")) {
          std::cout << "Checking variable: " << FactStr << std::endl;
          ResultNotEmpty = true;
          EXPECT_EQ(std::get<3>(Truth), Value);
        }
      }
    }
    EXPECT_TRUE(ResultNotEmpty);
  }

}; // Test Fixture

// Test for Case I of Store
TEST_F(InterMonoFullConstantPropagationTest, BasicTest_01) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>(
          "main", 5, "i", 13));
  doAnalysisAndCompareResults("basic_01_cpp.ll", GroundTruth, true);
}

// Test for Case II of Store and Load Inst
TEST_F(InterMonoFullConstantPropagationTest, BasicTest_02) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>(
          "main", 8, "i", 13));
  doAnalysisAndCompareResults("basic_02_cpp.ll", GroundTruth, true);
}

// Test for Operators
TEST_F(InterMonoFullConstantPropagationTest, BasicTest_03) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>(
          "main", 9, "i", 13));
  doAnalysisAndCompareResults("basic_03_cpp.ll", GroundTruth, true);
}

// Test for return Flow
TEST_F(InterMonoFullConstantPropagationTest, AdvancedTest_01) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>>(
          "main", 6, "i", 13));
  doAnalysisAndCompareResults("advanced_01_cpp.ll", GroundTruth, true);
}

TEST_F(InterMonoFullConstantPropagationTest, sqSubSetEqualTest) {
  InterMonoFullConstantPropagation FCP(nullptr, nullptr, nullptr, nullptr,
                                       EntryPoints);
  BitVectorSet<InterMonoFullConstantPropagation::d_t> set1;
  BitVectorSet<InterMonoFullConstantPropagation::d_t> set2;

  EXPECT_TRUE(FCP.sqSubSetEqual(set2, set1));

  set2.insert({nullptr, Top{}});

  EXPECT_FALSE(FCP.sqSubSetEqual(set2, set1));
}

TEST_F(InterMonoFullConstantPropagationTest, joinTest) {
  InterMonoFullConstantPropagation FCP(nullptr, nullptr, nullptr, nullptr,
                                       EntryPoints);
  BitVectorSet<InterMonoFullConstantPropagation::d_t> set1;
  BitVectorSet<InterMonoFullConstantPropagation::d_t> set2;
  BitVectorSet<InterMonoFullConstantPropagation::d_t> Out;

  set1.insert({nullptr, Top{}});
  Out = FCP.join(set1, set2);
  EXPECT_TRUE(set1 == Out);

  set2.insert({nullptr, 3});
  Out.clear();
  Out.insert({nullptr, 3});
  EXPECT_TRUE(Out == FCP.join(set1, set2));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
