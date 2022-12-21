/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"
#include "phasar/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"

#include "gtest/gtest.h"

#include "llvm/Support/raw_ostream.h"

#include <unordered_set>
#include <vector>

#include "TestConfig.h"

using namespace psr;
using namespace psr::glca;

using groundTruth_t =
    std::tuple<const IDEGeneralizedLCA::l_t, unsigned, unsigned>;

/* ============== TEST FIXTURE ============== */

class IDEGeneralizedLCATest : public ::testing::Test {

protected:
  const std::string PathToLLFiles =
      unittest::PathToLLTestFiles + "general_linear_constant/";

  std::unique_ptr<LLVMProjectIRDB> IRDB;
  std::unique_ptr<IDESolver<IDEGeneralizedLCADomain>> LCASolver;
  std::unique_ptr<LLVMTypeHierarchy> TH;
  std::unique_ptr<LLVMPointsToSet> PT;
  std::unique_ptr<LLVMBasedICFG> ICFG;
  std::unique_ptr<IDEGeneralizedLCA> LCAProblem;

  IDEGeneralizedLCATest() = default;

  void initialize(llvm::StringRef LLFile, size_t MaxSetSize = 2) {
    IRDB = std::make_unique<LLVMProjectIRDB>(PathToLLFiles + LLFile);
    TH = std::make_unique<LLVMTypeHierarchy>(*IRDB);
    PT = std::make_unique<LLVMPointsToSet>(*IRDB);
    ICFG = std::make_unique<LLVMBasedICFG>(
        IRDB.get(), CallGraphAnalysisType::RTA,
        std::vector<std::string>{"main"}, TH.get(), PT.get());
    LCAProblem = std::make_unique<IDEGeneralizedLCA>(
        IRDB.get(), ICFG.get(), std::vector<std::string>{"main"}, MaxSetSize);
    LCASolver = std::make_unique<IDESolver<IDEGeneralizedLCADomain>>(
        *LCAProblem, ICFG.get());

    LCASolver->solve();
  }

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {}

  //  compare results
  /// \brief compares the computed results with every given tuple (value,
  /// alloca, inst)
  void compareResults(const std::vector<groundTruth_t> &Expected) {
    for (const auto &[EVal, VrId, InstId] : Expected) {
      auto *Vr = IRDB->getInstruction(VrId);
      auto *Inst = IRDB->getInstruction(InstId);
      ASSERT_NE(nullptr, Vr);
      ASSERT_NE(nullptr, Inst);
      auto Result = LCASolver->resultAt(Inst, Vr);
      std::string Buffer;
      llvm::raw_string_ostream SStr(Buffer);
      LCASolver->dumpResults(SStr);
      EXPECT_EQ(EVal, Result)
          << "vr:" << VrId << " inst:" << InstId << " LCASolver:" << SStr.str();
    }
  }

}; // class Fixture

TEST_F(IDEGeneralizedLCATest, SimpleTest) {
  initialize("SimpleTest_c.ll");
  std::vector<groundTruth_t> GroundTruth;
  GroundTruth.push_back({{EdgeValue(10)}, 3, 20});
  GroundTruth.push_back({{EdgeValue(15)}, 4, 20});
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, BranchTest) {
  initialize("BranchTest_c.ll");
  std::vector<groundTruth_t> GroundTruth;
  GroundTruth.push_back({{EdgeValue(25), EdgeValue(43)}, 3, 22});
  GroundTruth.push_back({{EdgeValue(24)}, 4, 22});
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, FPtest) {
  initialize("FPtest_c.ll");
  std::vector<groundTruth_t> GroundTruth;
  GroundTruth.push_back({{EdgeValue(4.5)}, 1, 16});
  GroundTruth.push_back({{EdgeValue(2.0)}, 2, 16});
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTest) {
  initialize("StringTest_c.ll");
  std::vector<groundTruth_t> GroundTruth;
  GroundTruth.push_back({{EdgeValue("Hello, World")}, 2, 8});
  GroundTruth.push_back({{EdgeValue("Hello, World")}, 3, 8});
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringBranchTest) {
  initialize("StringBranchTest_c.ll");
  std::vector<groundTruth_t> GroundTruth;
  GroundTruth.push_back(
      {{EdgeValue("Hello, World"), EdgeValue("Hello Hello")}, 3, 15});
  GroundTruth.push_back({{EdgeValue("Hello Hello")}, 4, 15});
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTestCpp) {
  initialize("StringTest_cpp.ll");
  std::vector<groundTruth_t> GroundTruth;
  const auto *LastMainInstruction =
      getLastInstructionOf(IRDB->getFunction("main"));
  GroundTruth.push_back({{EdgeValue("Hello, World")},
                         3,
                         std::stoi(getMetaDataID(LastMainInstruction))});
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, FloatDivisionTest) {
  initialize("FloatDivision_c.ll");
  std::vector<groundTruth_t> GroundTruth;
  GroundTruth.push_back({{EdgeValue(nullptr)}, 1, 24}); // i
  GroundTruth.push_back({{EdgeValue(1.0)}, 2, 24});     // j
  GroundTruth.push_back({{EdgeValue(-7.0)}, 3, 24});    // k
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, SimpleFunctionTest) {
  initialize("SimpleFunctionTest_c.ll");
  std::vector<groundTruth_t> GroundTruth;
  GroundTruth.push_back({{EdgeValue(48)}, 10, 31});      // i
  GroundTruth.push_back({{EdgeValue(nullptr)}, 11, 31}); // j
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, GlobalVariableTest) {
  initialize("GlobalVariableTest_c.ll");
  std::vector<groundTruth_t> GroundTruth;
  GroundTruth.push_back({{EdgeValue(50)}, 7, 13}); // i
  GroundTruth.push_back({{EdgeValue(8)}, 10, 13}); // j
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, Imprecision) {
  initialize("Imprecision_c.ll", 2);
  //   auto xInst = IRDB->getInstruction(0); // foo.x
  //   auto yInst = IRDB->getInstruction(1); // foo.y
  //  auto barInst = IRDB->getInstruction(7);

  // llvm::outs() << "foo.x = " << LCASolver->resultAt(barInst, xInst) <<
  // std::endl; llvm::outs() << "foo.y = " << LCASolver->resultAt(barInst,
  // yInst)
  // << std::endl;

  std::vector<groundTruth_t> GroundTruth;
  GroundTruth.push_back({{EdgeValue(1), EdgeValue(2)}, 0, 7}); // i
  GroundTruth.push_back({{EdgeValue(2), EdgeValue(3)}, 1, 7}); // j
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, ReturnConstTest) {
  initialize("ReturnConstTest_c.ll");
  std::vector<groundTruth_t> GroundTruth;
  GroundTruth.push_back({{EdgeValue(43)}, 7, 8}); // i
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, NullTest) {
  initialize("NullTest_c.ll");
  std::vector<groundTruth_t> GroundTruth;
  GroundTruth.push_back({{EdgeValue("")}, 4, 5}); // foo(null)
  compareResults(GroundTruth);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
