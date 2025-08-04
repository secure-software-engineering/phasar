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
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_ostream.h"

#include "SourceMapping.h"
#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <vector>

using namespace psr;
using namespace psr::glca;

using groundTruth_t = std::tuple<const IDEGeneralizedLCA::l_t,
                                 SrcCodeLocationEntry, SrcCodeLocationEntry>;

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
  void compareResults(std::vector<groundTruth_t> &Expected) {
    for (const auto &Entry : Expected) {
      const auto &EVal = std::get<0>(Entry);
      const auto &SCLVr = std::get<1>(Entry);
      const auto &SCLInst = std::get<2>(Entry);

      const auto *Vr = getInstLambdaOrNot(SCLVr);
      const auto *Inst = getInstLambdaOrNot(SCLInst);

      bool Flag = false;

      if (Vr) {
        llvm::outs() << "VrId Inst:   " << *Vr << "\n";
      } else {
        llvm::outs() << "VrId is nullptr\n";
        Flag = true;
      }

      if (Inst) {
        llvm::outs() << "InstID Inst: " << *Inst << "\n";
      } else {
        llvm::outs() << "Inst is nullptr\n";
        Flag = true;
      }

      if (Flag) {
        EXPECT_TRUE(false);
        continue;
      }

      llvm::outs() << "Result: " << LCASolver->resultAt(Inst, Vr) << "\n";

      ASSERT_NE(nullptr, Vr);
      ASSERT_NE(nullptr, Inst);

      auto Result = LCASolver->resultAt(Inst, Vr);
      EXPECT_EQ(EVal, Result)
          << "vr:" << Vr->getValueID() << " inst:" << Inst->getValueID()
          << " Expected: " << EVal << " Got:" << Result;
    }
  }

private:
  const llvm::Instruction *
  getInstLambdaOrNot(const SrcCodeLocationEntry &Entry) {
    if (Entry.LambdaFunc) {
      return unittest::getInstAtOrNull(
          std::get<const llvm::Function *>(Entry.Context), Entry.Line,
          Entry.Column, Entry.LambdaFunc);
    }

    return unittest::getInstAtOrNull(
        std::get<const llvm::Function *>(Entry.Context), Entry.Line,
        Entry.Column);
  }
}; // class Fixture

TEST_F(IDEGeneralizedLCATest, SimpleTest) {
  initialize("SimpleTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(10)},
       SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(7, 3, HA->getProjectIRDB().getFunction("main"))});
  GroundTruth.push_back(
      {{EdgeValue(15)},
       SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(7, 3, HA->getProjectIRDB().getFunction("main"))});

  compareResults(GroundTruth);
}
TEST_F(IDEGeneralizedLCATest, BranchTest) {
  initialize("BranchTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(25)},
       SrcCodeLocationEntry(7, 11, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(8, 3, HA->getProjectIRDB().getFunction("main"))});
  GroundTruth.push_back(
      {{EdgeValue(24)},
       SrcCodeLocationEntry(7, 9, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(8, 3, HA->getProjectIRDB().getFunction("main"))});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, FPtest) {
  initialize("FPtest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(4.5)},
       SrcCodeLocationEntry(4, 9, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(6, 3, HA->getProjectIRDB().getFunction("main"))});
  GroundTruth.push_back(
      {{EdgeValue(2.0)},
       SrcCodeLocationEntry(5, 9, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(6, 3, HA->getProjectIRDB().getFunction("main"))});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTest) {
  initialize("StringTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue("Hello, World")},
       SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(7, 3, HA->getProjectIRDB().getFunction("main"))});
  GroundTruth.push_back(
      {{EdgeValue("Hello, World")},
       SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(7, 3, HA->getProjectIRDB().getFunction("main"))});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringBranchTest) {
  initialize("StringBranchTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue("Hello Hello"), EdgeValue("Hello, World")},
       SrcCodeLocationEntry(5, 15, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(10, 3, HA->getProjectIRDB().getFunction("main"))});
  GroundTruth.push_back(
      {{EdgeValue("Hello Hello")},
       SrcCodeLocationEntry(6, 15, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(10, 3, HA->getProjectIRDB().getFunction("main"))});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTestCpp) {
  initialize("StringTest_cpp_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue("Hello, World")},
       SrcCodeLocationEntry(4, 15, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(6, 1, HA->getProjectIRDB().getFunction("main"))});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, FloatDivisionTest) {
  initialize("FloatDivision_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(1.0)},
       SrcCodeLocationEntry(5, 9, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(8, 3, HA->getProjectIRDB().getFunction("main"))});
  GroundTruth.push_back(
      {{EdgeValue(nullptr)},
       SrcCodeLocationEntry(6, 9, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(8, 3, HA->getProjectIRDB().getFunction("main"))});
  GroundTruth.push_back(
      {{EdgeValue(-7.0)},
       SrcCodeLocationEntry(7, 9, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(8, 3, HA->getProjectIRDB().getFunction("main"))});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, SimpleFunctionTest) {
  initialize("SimpleFunctionTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(48)},
       SrcCodeLocationEntry(8, 7, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(10, 3, HA->getProjectIRDB().getFunction("main"))});
  GroundTruth.push_back(
      {{EdgeValue(nullptr)},
       SrcCodeLocationEntry(9, 7, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(10, 3, HA->getProjectIRDB().getFunction("main"))});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, GlobalVariableTest) {
  initialize("GlobalVariableTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(50)},
       SrcCodeLocationEntry(4, 13, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(6, 3, HA->getProjectIRDB().getFunction("main"))});
  GroundTruth.push_back(
      {{EdgeValue(8)},
       SrcCodeLocationEntry(5, 13, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(6, 3, HA->getProjectIRDB().getFunction("main"))});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, Imprecision) {
  initialize("Imprecision_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(1), EdgeValue(2)},
       SrcCodeLocationEntry(3, 14, HA->getProjectIRDB().getFunction("foo")),
       SrcCodeLocationEntry(3, 26, HA->getProjectIRDB().getFunction("foo"))});
  GroundTruth.push_back(
      {{EdgeValue(2), EdgeValue(3)},
       SrcCodeLocationEntry(3, 21, HA->getProjectIRDB().getFunction("foo")),
       SrcCodeLocationEntry(3, 26, HA->getProjectIRDB().getFunction("foo"))});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, ReturnConstTest) {
  initialize("ReturnConstTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue(43)},
       SrcCodeLocationEntry(6, 12, HA->getProjectIRDB().getFunction("main")),
       SrcCodeLocationEntry(6, 3, HA->getProjectIRDB().getFunction("main"))});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, NullTest) {
  initialize("NullTest_c_dbg.ll");
  std::vector<groundTruth_t> GroundTruth;

  GroundTruth.push_back(
      {{EdgeValue("")},
       SrcCodeLocationEntry(1, 31, HA->getProjectIRDB().getFunction("foo")),
       SrcCodeLocationEntry(1, 24, HA->getProjectIRDB().getFunction("foo"))});

  compareResults(GroundTruth);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
