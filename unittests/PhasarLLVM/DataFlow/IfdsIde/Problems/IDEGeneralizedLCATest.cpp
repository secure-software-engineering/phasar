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
#include "phasar/Utils/SrcCodeLocationEntry.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include "SourceMapping.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <vector>

using namespace psr;
using namespace psr::glca;

using groundTruth_t =
    std::tuple<const IDEGeneralizedLCA::l_t, unsigned, unsigned>;

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
  void compareResults(
      std::vector<std::tuple<SrcCodeLocationEntry,
                             const IDEGeneralizedLCA::l_t>> &Expected) {
    for (const auto &Entry : Expected) {
      const auto &SCLEntry = std::get<0>(Entry);
      const auto *Vr = unittest::getInstAtOrNull(
          std::get<const llvm::Function *>(SCLEntry.Context), SCLEntry.Line,
          SCLEntry.Column);
      const auto *Inst = unittest::getInstAtOrNull(
          std::get<const llvm::Function *>(SCLEntry.Context), SCLEntry.Line,
          SCLEntry.Column);

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

      ASSERT_NE(nullptr, Vr);
      ASSERT_NE(nullptr, Inst);

      auto Result = LCASolver->resultAt(Inst, Vr);
      auto EVal = std::get<1>(Entry);
      EXPECT_EQ(EVal, Result)
          << "vr:" << Vr->getValueID() << " inst:" << Inst->getValueID()
          << " Expected: " << EVal << " Got:" << Result;
    }

    /*for (const auto &[EVal, VrId, InstId] : Expected) {
      const auto *Vr = HA->getProjectIRDB().getInstruction(VrId);
      const auto *Inst = HA->getProjectIRDB().getInstruction(InstId);
      llvm::outs() << "VrId Inst:   " << *Vr << "\n";
      llvm::outs() << "InstID Inst: " << *Inst << "\n";
      ASSERT_NE(nullptr, Vr);
      ASSERT_NE(nullptr, Inst);
      auto Result = LCASolver->resultAt(Inst, Vr);

      EXPECT_EQ(EVal, Result) << "vr:" << VrId << " inst:" << InstId
                              << " Expected: " << EVal << " Got:" << Result;
    }*/
  }

}; // class Fixture

TEST_F(IDEGeneralizedLCATest, SimpleTest) {
  initialize("SimpleTest_c_dbg.ll");

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;

  /*
    TODO: ask Fabian why the edge values from the previous ground truths do not
    work anymore. An example result:
    vr:58 inst:58 Expected: {15} Got:{<TOP>}

    Also, ask how to determine the correct EdgeValue.
  */

  GroundTruth.push_back(
      {SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(10)}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(6, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(15)}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, BranchTest) {
  initialize("BranchTest_c_dbg.ll");
  // TODO: Test fails. An example result:
  // vr:59 inst:59 Expected: {24} Got:{<TOP>}

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;
  GroundTruth.push_back(
      {SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(25)}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(7, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(24)}});
  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, FPtest) {
  initialize("FPtest_c_dbg.ll");
  // TODO: Test fails. An example result:
  // vr:59 inst:59 Expected: {2.000000e+00} Got:{<TOP>}

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;
  GroundTruth.push_back(
      {SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(4.5)}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(2.0)}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTest) {
  initialize("StringTest_c_dbg.ll");
  // TODO: Test fails. An example result:
  // vr:58 inst:58 Expected: {"Hello, World"} Got:{<TOP>}

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;
  GroundTruth.push_back(
      {SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue("Hello, World")}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue("Hello, World")}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringBranchTest) {
  initialize("StringBranchTest_c_dbg.ll");
  // TODO: Test fails. An example result:
  // vr:59 inst:59 Expected: {"Hello, World"} Got:{<TOP>}

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;
  /*
    TODO: check which version is correct here
  */
#if false
  GroundTruth.push_back(
      {SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue("Hello, World"), EdgeValue("Hello, World")}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue("Hello, World")}});
#endif

  GroundTruth.push_back(
      {SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue("Hello, World")}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue("Hello, World")}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(8, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue("Hello, World")}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, StringTestCpp) {
  initialize("StringTest_cpp_dbg.ll");
  // TODO: Test fails. An example result:
  // vr:58 inst:58 Expected: {"Hello, World"} Got:{<TOP>}

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;
  GroundTruth.push_back(
      {SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue("Hello, World")}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, FloatDivisionTest) {
  initialize("FloatDivision_c_dbg.ll");
  // TODO: Test fails. An example result:
  // vr:59 inst:59 Expected: {-7.000000e+00} Got:{<TOP>}

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;
  GroundTruth.push_back(
      {SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(nullptr)}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(6, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(1.0)}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(7, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(-7.0)}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, SimpleFunctionTest) {
  initialize("SimpleFunctionTest_c_dbg.ll");
  // TODO: Test fails. An example result:
  // vr:59 inst:59 Expected: {48} Got:{<TOP>}

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;
  GroundTruth.push_back(
      {SrcCodeLocationEntry(8, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(48)}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(9, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(nullptr)}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, GlobalVariableTest) {
  initialize("GlobalVariableTest_c_dbg.ll");
  // TODO: Test fails. An example result:
  // vr:58 inst:58 Expected: {8} Got:{<TOP>}

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;
  GroundTruth.push_back(
      {SrcCodeLocationEntry(4, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(50)}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(8)}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, Imprecision) {
  initialize("Imprecision_c_dbg.ll");
  // TODO: Test fails. An example result:
  // vr:83 inst:83 Expected: {3, 2} Got:{<TOP>}

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;
  GroundTruth.push_back(
      {SrcCodeLocationEntry(6, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(1), EdgeValue(2)}});
  GroundTruth.push_back(
      {SrcCodeLocationEntry(7, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(2), EdgeValue(3)}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, ReturnConstTest) {
  initialize("ReturnConstTest_c_dbg.ll");
  // TODO: Test fails. An example result:
  // vr:59 inst:59 Expected: {43} Got:{<TOP>}

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;
  GroundTruth.push_back(
      {SrcCodeLocationEntry(6, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue(43)}});

  compareResults(GroundTruth);
}

TEST_F(IDEGeneralizedLCATest, NullTest) {
  initialize("NullTest_c_dbg.ll");
  // TODO: Test fails. An example result:
  // vr:83 inst:83 Expected: {""} Got:{<TOP>}

  std::vector<std::tuple<SrcCodeLocationEntry, const IDEGeneralizedLCA::l_t>>
      GroundTruth;
  GroundTruth.push_back(
      {SrcCodeLocationEntry(5, 0, HA->getProjectIRDB().getFunction("main")),
       {EdgeValue("")}});

  compareResults(GroundTruth);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
