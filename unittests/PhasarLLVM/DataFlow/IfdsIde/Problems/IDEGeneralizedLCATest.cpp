/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"

#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <vector>

using namespace psr;
using namespace psr::glca;
using namespace psr::unittest;

using l_t = IDEGeneralizedLCA::l_t;
using groundTruth_t = std::tuple<l_t, TestingSrcLocation, TestingSrcLocation>;

/* ============== TEST FIXTURE ============== */

class IDEGeneralizedLCATest : public ::testing::Test {

protected:
  static constexpr auto PathToLLFiles =
      PHASAR_BUILD_SUBFOLDER("general_linear_constant/");

  std::optional<HelperAnalyses> HA;
  std::optional<IDEGeneralizedLCA> LCAProblem;
  std::unique_ptr<IDESolver<IDEGeneralizedLCADomain>> LCASolver;

  static constexpr size_t MaxSetSize = 2;

  IDEGeneralizedLCATest() = default;

  void initialize(llvm::StringRef LLFile, size_t MaxSetSize = 2) {
    using namespace std::literals;
    HA.emplace(PathToLLFiles + LLFile, std::vector{"main"s});
    LCAProblem = createAnalysisProblem<IDEGeneralizedLCA>(
        *HA, std::vector{"main"s}, MaxSetSize);
    LCASolver = std::make_unique<IDESolver<IDEGeneralizedLCADomain>>(
        *LCAProblem, &HA->getICFG());

    LCASolver->solve();
  }

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {}

  //  compare results
  /// \brief compares the computed results with every given tuple (value,
  /// alloca, inst)
  void compareResults(const std::vector<groundTruth_t> &Expected) {
    for (const auto &[EVal, VarLoc, InstLoc] : Expected) {
      const auto *Var = testingLocInIR(VarLoc, HA->getProjectIRDB());
      const auto *Inst = llvm::dyn_cast_if_present<llvm::Instruction>(
          testingLocInIR(InstLoc, HA->getProjectIRDB()));

      ASSERT_TRUE(Var) << "Cannot map location " << VarLoc.str() << " to LLVM";
      ASSERT_TRUE(Inst) << "Cannot map location " << InstLoc.str()
                        << " to LLVM";

      auto Result = LCASolver->resultAt(Inst, Var);
      EXPECT_EQ(EVal, Result)
          << "At VarLoc: " << VarLoc.str() << ", InstLoc: " << InstLoc.str()
          << ";\n  aka. Var: " << llvmIRToString(Var)
          << "; Inst: " << llvmIRToString(Inst) << ":\n  Expected: " << EVal
          << " Got:" << LToString(Result);
    }
  }

}; // class Fixture

TEST_F(IDEGeneralizedLCATest, SimpleTest) {
  initialize("SimpleTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.emplace_back(l_t{EdgeValue(10)}, LineColFun{4, 0, "main"},
                           LineColFun{7, 3, "main"});
  GroundTruth.emplace_back(l_t{EdgeValue(15)}, LineColFun{5, 0, "main"},
                           LineColFun{7, 3, "main"});

  compareResults(GroundTruth);
}
TEST_F(IDEGeneralizedLCATest, BranchTest) {
  initialize("BranchTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(25)}, LineColFun{7, 11, "main"}, LineColFun{8, 3, "main"}});
  GroundTruth.push_back(
      {{EdgeValue(24)}, LineColFun{7, 9, "main"}, LineColFun{8, 3, "main"}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, FPtest) {
  initialize("FPtest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(4.5)}, LineColFun{4, 9, "main"}, LineColFun{6, 3, "main"}});
  GroundTruth.push_back(
      {{EdgeValue(2.0)}, LineColFun{5, 9, "main"}, LineColFun{6, 3, "main"}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTest) {
  initialize("StringTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back({{EdgeValue("Hello, World")},
                         LineColFun{4, 0, "main"},
                         LineColFun{7, 3, "main"}});
  GroundTruth.push_back({{EdgeValue("Hello, World")},
                         LineColFun{5, 0, "main"},
                         LineColFun{7, 3, "main"}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringBranchTest) {
  initialize("StringBranchTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back({{EdgeValue("Hello Hello"), EdgeValue("Hello, World")},
                         LineColFun{5, 15, "main"},
                         LineColFun{10, 3, "main"}});
  GroundTruth.push_back({{EdgeValue("Hello Hello")},
                         LineColFun{6, 15, "main"},
                         LineColFun{10, 3, "main"}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTestCpp) {
  initialize("StringTest_cpp_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back({{EdgeValue("Hello, World")},
                         LineColFun{4, 15, "main"},
                         LineColFun{6, 1, "main"}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, FloatDivisionTest) {
  initialize("FloatDivision_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(1.0)}, LineColFun{5, 9, "main"}, LineColFun{8, 3, "main"}});
  GroundTruth.push_back({{EdgeValue(nullptr)},
                         LineColFun{6, 9, "main"},
                         LineColFun{8, 3, "main"}});
  GroundTruth.push_back(
      {{EdgeValue(-7.0)}, LineColFun{7, 9, "main"}, LineColFun{8, 3, "main"}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, SimpleFunctionTest) {
  initialize("SimpleFunctionTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(48)}, LineColFun{8, 7, "main"}, LineColFun{10, 3, "main"}});
  GroundTruth.push_back({{EdgeValue(nullptr)},
                         LineColFun{9, 7, "main"},
                         LineColFun{10, 3, "main"}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, GlobalVariableTest) {
  initialize("GlobalVariableTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(50)}, LineColFun{4, 13, "main"}, LineColFun{6, 3, "main"}});
  GroundTruth.push_back(
      {{EdgeValue(8)}, LineColFun{5, 13, "main"}, LineColFun{6, 3, "main"}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, Imprecision) {
  initialize("Imprecision_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back({{EdgeValue(1), EdgeValue(2)},
                         LineColFun{3, 14, "foo"},
                         LineColFun{3, 26, "foo"}});
  GroundTruth.push_back({{EdgeValue(2), EdgeValue(3)},
                         LineColFun{3, 21, "foo"},
                         LineColFun{3, 26, "foo"}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, ReturnConstTest) {
  initialize("ReturnConstTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(43)}, LineColFun{6, 12, "main"}, LineColFun{6, 3, "main"}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, NullTest) {
  initialize("NullTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue("")}, LineColFun{1, 31, "foo"}, LineColFun{1, 24, "foo"}});

  compareResults(GroundTruth);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
