/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <iostream>
#include <unordered_set>
#include <vector>

#include "gtest/gtest.h"

#include "llvm/Support/raw_ostream.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/Logger.h"

using namespace psr;

typedef std::tuple<const IDEGeneralizedLCA::l_t, unsigned, unsigned>
    groundTruth_t;

/* ============== TEST FIXTURE ============== */

class IDEGeneralizedLCATest : public ::testing::Test {

protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/general_linear_constant/";

  ProjectIRDB *IRDB = nullptr;
  IDESolver<IDEGeneralizedLCADomain> *LCASolver = nullptr;

  IDEGeneralizedLCATest() {}
  virtual ~IDEGeneralizedLCATest() {}

  void Initialize(const std::string &llFile, size_t maxSetSize = 2) {
    IRDB = new ProjectIRDB({pathToLLFiles + llFile}, IRDBOptions::WPA);
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToSet PT(*IRDB);
    LLVMBasedICFG ICFG(*IRDB, CallGraphAnalysisType::RTA, {"main"}, &TH, &PT);
    IDEGeneralizedLCA LCAProblem(IRDB, &TH, &ICFG, &PT, {"main"}, maxSetSize);
    LCASolver = new IDESolver(LCAProblem);

    LCASolver->solve();
  }

  void SetUp() override {
    boost::log::core::get()->set_logging_enabled(false);
    ValueAnnotationPass::resetValueID();
  }

  void TearDown() override {
    delete IRDB;
    delete LCASolver;
  }

  //  compare results
  /// \brief compares the computed results with every given tuple (value,
  /// alloca, inst)
  void compareResults(const std::vector<groundTruth_t> &expected) {
    for (auto &[val, vrId, instId] : expected) {
      auto vr = IRDB->getInstruction(vrId);
      auto inst = IRDB->getInstruction(instId);
      ASSERT_NE(nullptr, vr);
      ASSERT_NE(nullptr, inst);
      auto result = LCASolver->resultAt(inst, vr);
      EXPECT_EQ(val, result);
    }
  }

}; // class Fixture

TEST_F(IDEGeneralizedLCATest, SimpleTest) {
  Initialize("SimpleTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(10)}, 3, 20});
  groundTruth.push_back({{EdgeValue(15)}, 4, 20});
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, BranchTest) {
  Initialize("BranchTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(25), EdgeValue(43)}, 3, 22});
  groundTruth.push_back({{EdgeValue(24)}, 4, 22});
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, FPtest) {
  Initialize("FPtest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(4.5)}, 1, 16});
  groundTruth.push_back({{EdgeValue(2.0)}, 2, 16});
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTest) {
  Initialize("StringTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue("Hello, World")}, 2, 8});
  groundTruth.push_back({{EdgeValue("Hello, World")}, 3, 8});
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringBranchTest) {
  Initialize("StringBranchTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back(
      {{EdgeValue("Hello, World"), EdgeValue("Hello Hello")}, 3, 15});
  groundTruth.push_back({{EdgeValue("Hello Hello")}, 4, 15});
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTestCpp) {
  Initialize("StringTest_cpp.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue("Hello, World")}, 2, 13});
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, FloatDivisionTest) {
  Initialize("FloatDivision_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(nullptr)}, 1, 24}); // i
  groundTruth.push_back({{EdgeValue(1.0)}, 2, 24});     // j
  groundTruth.push_back({{EdgeValue(-7.0)}, 3, 24});    // k
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, SimpleFunctionTest) {
  Initialize("SimpleFunctionTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(48)}, 10, 31});      // i
  groundTruth.push_back({{EdgeValue(nullptr)}, 11, 31}); // j
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, GlobalVariableTest) {
  Initialize("GlobalVariableTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(50)}, 7, 13});       // i
  groundTruth.push_back({{EdgeValue(nullptr)}, 10, 13}); // j
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, Imprecision) {
  // bl::core::get()->set_logging_enabled(true);
  Initialize("Imprecision_c.ll", 2);
  auto xInst = IRDB->getInstruction(0); // foo.x
  auto yInst = IRDB->getInstruction(1); // foo.y
  auto barInst = IRDB->getInstruction(7);

  // std::cout << "foo.x = " << LCASolver->resultAt(barInst, xInst) <<
  // std::endl; std::cout << "foo.y = " << LCASolver->resultAt(barInst, yInst)
  // << std::endl;

  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(1), EdgeValue(2)}, 0, 7}); // i
  groundTruth.push_back({{EdgeValue(2), EdgeValue(3)}, 1, 7}); // j
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, ReturnConstTest) {
  Initialize("ReturnConstTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue(43)}, 7, 8}); // i
  compareResults(groundTruth);
}

TEST_F(IDEGeneralizedLCATest, NullTest) {
  Initialize("NullTest_c.ll");
  std::vector<groundTruth_t> groundTruth;
  groundTruth.push_back({{EdgeValue("")}, 4, 5}); // foo(null)
  compareResults(groundTruth);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
