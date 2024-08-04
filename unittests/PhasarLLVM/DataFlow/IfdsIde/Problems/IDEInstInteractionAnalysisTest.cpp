/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEInstInteractionAnalysis.h"

#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/HelperAnalysisConfig.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <variant>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEInstInteractionAnalysisTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("inst_interaction/");

  // Function - Line Nr - Variable - Values
  using IIACompactResult_t =
      std::tuple<std::string, std::size_t, std::string,
                 IDEInstInteractionAnalysisT<std::string, true>::l_t>;

  std::optional<HelperAnalyses> HA;
  LLVMProjectIRDB *IRDB{};

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void initializeIR(const std::string &LlvmFilePath,
                    const std::vector<std::string> &EntryPoints = {"main"}) {
    HA.emplace(PathToLlFiles + LlvmFilePath, EntryPoints,
               HelperAnalysisConfig{}.withCGType(CallGraphAnalysisType::CHA));
    IRDB = &HA->getProjectIRDB();
  }

  void
  doAnalysisAndCompareResults(const std::string &LlvmFilePath,
                              const std::vector<std::string> &EntryPoints,
                              const std::set<IIACompactResult_t> &GroundTruth,
                              bool PrintDump = false) {
    initializeIR(LlvmFilePath, EntryPoints);
    if (PrintDump) {
      IRDB->emitPreprocessedIR(llvm::outs());
    }

    // IDEInstInteractionAnalysisT<std::string, true> IIAProblem(IRDB, &ICFG,
    // &PT,
    //                                                           EntryPoints);
    assert(HA);
    auto IIAProblem =
        createAnalysisProblem<IDEInstInteractionAnalysisT<std::string>>(
            *HA, EntryPoints);
    // use Phasar's instruction ids as testing labels
    auto Generator =
        [](std::variant<const llvm::Instruction *, const llvm::GlobalVariable *>
               Current) -> std::set<std::string> {
      return std::visit(
          [](const auto *InstOrGlob) -> std::set<std::string> {
            std::set<std::string> Labels;
            if (InstOrGlob->hasMetadata()) {
              std::string Label =
                  llvm::cast<llvm::MDString>(
                      InstOrGlob->getMetadata(PhasarConfig::MetaDataKind())
                          ->getOperand(0))
                      ->getString()
                      .str();
              Labels.insert(Label);
            }
            return Labels;
          },
          Current);
    };
    // register the above generator function
    IIAProblem.registerEdgeFactGenerator(Generator);
    IDESolver IIASolver(IIAProblem, &HA->getICFG());
    IIASolver.solve();
    if (PrintDump) {
      IIASolver.dumpResults();
    }
    // do the comparison
    for (const auto &[FunName, SrcLine, VarName, LatticeVal] : GroundTruth) {
      const auto *Fun = IRDB->getFunctionDefinition(FunName);
      const auto *IRLine = getNthInstruction(Fun, SrcLine);
      auto ResultMap = IIASolver.resultsAt(IRLine);
      assert(IRLine && "Could not retrieve IR line!");
      bool FactFound = false;
      for (auto &[Fact, Value] : ResultMap) {
        std::string FactStr;
        llvm::raw_string_ostream RSO(FactStr);
        RSO << *Fact.getBase();
        llvm::StringRef FactRef(FactStr);
        if (FactRef.ltrim().startswith("%" + VarName + " ") ||
            FactRef.ltrim().startswith("@" + VarName + " ")) {
          PHASAR_LOG_LEVEL(DFADEBUG, "Checking variable: " << FactStr);
          EXPECT_EQ(LatticeVal, Value);
          FactFound = true;
        }
      }
      if (!FactFound) {
        PHASAR_LOG_LEVEL(DFADEBUG, "Variable '" << VarName << "' missing at '"
                                                << llvmIRToString(IRLine)
                                                << "'.");
      }
      EXPECT_TRUE(FactFound);
    }
  }

  void TearDown() override {}

}; // Test Fixture

TEST_F(IDEInstInteractionAnalysisTest, FieldSensArrayConstruction_01) {
  initializeIR("array_01_cpp.ll");
  const auto *Main = IRDB->getFunction("main");
  const auto *Inst = getNthInstruction(Main, 2);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 13);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 16);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  ASSERT_TRUE(true);
}

TEST_F(IDEInstInteractionAnalysisTest, FieldSensArrayConstruction_02) {
  initializeIR("array_02_cpp.ll");
  const auto *Main = IRDB->getFunction("main");
  const auto *Inst = getNthInstruction(Main, 2);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 3);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 5);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 6);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  ASSERT_TRUE(true);
}

TEST_F(IDEInstInteractionAnalysisTest, FieldSensArrayConstruction_03) {
  initializeIR("array_03_cpp.ll");
  const auto *Main = IRDB->getFunction("main");
  const auto *Inst = getNthInstruction(Main, 2);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 5);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 6);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 7);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  ASSERT_TRUE(true);
}

TEST_F(IDEInstInteractionAnalysisTest, FieldSensStructConstruction_01) {
  initializeIR("struct_01_cpp.ll");
  const auto *Main = IRDB->getFunction("main");
  const auto *Inst = getNthInstruction(Main, 2);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 14);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 17);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  ASSERT_TRUE(true);
}

TEST_F(IDEInstInteractionAnalysisTest, FieldSensStructConstruction_02) {
  initializeIR("struct_02_cpp.ll");
  const auto *Main = IRDB->getFunction("main");
  const auto *Inst = getNthInstruction(Main, 2);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 6);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 7);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getNthInstruction(Main, 9);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  ASSERT_TRUE(true);
}

TEST_F(IDEInstInteractionAnalysisTest, ArrayEquality_01) {
  initializeIR("array_01_cpp.ll");

  const auto *Main = IRDB->getFunction("main");
  const auto *Inst = getNthInstruction(Main, 2);
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, FlowFact);

  Inst = getNthInstruction(Main, 4);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 13);
  auto OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_NE(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 13);

  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 18);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 16);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 21);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);
}

TEST_F(IDEInstInteractionAnalysisTest, ArrayEquality_02) {
  initializeIR("array_02_cpp.ll");
  const auto *Main = IRDB->getFunction("main");
  const auto *Inst = getNthInstruction(Main, 2);
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, FlowFact);

  Inst = getNthInstruction(Main, 5);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 8);
  auto OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 6);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 9);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 5);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 6);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_NE(FlowFact, OtherFlowFact);
}

TEST_F(IDEInstInteractionAnalysisTest, ArrayEquality_03) {
  initializeIR("array_03_cpp.ll");
  const auto *Main = IRDB->getFunction("main");
  const auto *Inst = getNthInstruction(Main, 2);
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, FlowFact);

  Inst = getNthInstruction(Main, 5);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 9);
  auto OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 6);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 10);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 7);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 11);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 6);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 11);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  // For K-limit of 2, this should be considered equal
  if (IDEIIAFlowFact::KLimit <= 2) {
    ASSERT_EQ(FlowFact, OtherFlowFact);
  } else {
    ASSERT_NE(FlowFact, OtherFlowFact);
  }
}

TEST_F(IDEInstInteractionAnalysisTest, StructEquality_01) {
  initializeIR("struct_01_cpp.ll");
  const auto *Main = IRDB->getFunction("main");
  const auto *Inst = getNthInstruction(Main, 2);
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, FlowFact);

  Inst = getNthInstruction(Main, 14);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 22);
  auto OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 17);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 25);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 20);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 28);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 17);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 20);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << "Compare:\n";
  llvm::outs() << FlowFact << '\n';
  llvm::outs() << OtherFlowFact << '\n';
  // FIXME
  // ASSERT_NE(FlowFact, OtherFlowFact);
}

TEST_F(IDEInstInteractionAnalysisTest, StructEquality_02) {
  initializeIR("struct_02_cpp.ll");
  const auto *Main = IRDB->getFunction("main");
  const auto *Inst = getNthInstruction(Main, 2);
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, FlowFact);

  Inst = getNthInstruction(Main, 6);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 11);
  auto OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 6);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 7);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_NE(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 7);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 12);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 9);
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 15);
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getNthInstruction(Main, 6);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getNthInstruction(Main, 9);
  llvm::outs() << "Instruction to create flow fact from 2: " << *Inst << '\n';
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_NE(FlowFact, OtherFlowFact);
}

// TODO
// TEST_F(IDEInstInteractionAnalysisTest, HandleArrayTest_01) {
//   std::set<IIACompactResult_t> GroundTruth;
//   //   GroundTruth.emplace(
//   //   std::tuple<std::string, size_t, std::string,
//   BitVectorSet<std::string>>(
//   //   "main", 9, "i", {"4", "5"}));
//   doAnalysisAndCompareResults("array_01_cpp.ll", GroundTruth, true);
// }

// TEST_F(IDEInstInteractionAnalysisTest, HandleArrayTest_02) {
//   std::set<IIACompactResult_t> GroundTruth;
//   //   GroundTruth.emplace(
//   //   std::tuple<std::string, size_t, std::string,
//   BitVectorSet<std::string>>(
//   //   "main", 9, "i", {"4", "5"}));
//   doAnalysisAndCompareResults("array_02_cpp.ll", GroundTruth, false);
// }

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "i", {"4"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "j", {"4", "5", "6", "7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "retval", {"3"}));
  doAnalysisAndCompareResults("basic_01_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "retval", {"6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "argc.addr", {"7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "argv.addr", {"8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "i", {"16", "18"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "j", {"9", "10", "11", "12"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "k", {"21", "16", "18", "20"}));
  doAnalysisAndCompareResults("basic_02_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "retval", {"3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "i", {"4", "10", "11", "12"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "x", {"5", "14", "15", "16"}));
  doAnalysisAndCompareResults("basic_03_cpp.ll", {"main"}, GroundTruth, false);
}

PHASAR_SKIP_TEST(TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_04) {
  // If we use libcxx this won't work since internal implementation is different
  LIBCPP_GTEST_SKIP;

  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 23, "retval", {"13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 23, "argc.addr", {"14"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 23, "argv.addr", {"15"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 23, "i", {"16"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 23, "j", {"16", "17", "19", "18"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 23, "k", {"16", "17", "18", "19", "20", "24", "25"}));
  doAnalysisAndCompareResults("basic_04_cpp.ll", {"main"}, GroundTruth, false);
})

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 11, "i", {"5", "7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 11, "retval", {"2"}));
  doAnalysisAndCompareResults("basic_05_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_06) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "retval", {"5"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "i", {"15", "6", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "j", {"15", "6", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "k", {"6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 19, "p", {"1", "2", "9", "11"}));
  doAnalysisAndCompareResults("basic_06_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_07) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "retval", {"5"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "argc.addr", {"6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "argv.addr", {"7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "i", {"12"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 15, "j", {"8", "9", "10", "11"}));
  doAnalysisAndCompareResults("basic_07_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_08) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "retval", {"2"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "i", {"9"}));
  doAnalysisAndCompareResults("basic_08_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_09) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "i", {"4"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "j", {"4", "6", "7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "retval", {"3"}));
  doAnalysisAndCompareResults("basic_09_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_10) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "i", {"3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "retval", {"2"}));
  doAnalysisAndCompareResults("basic_10_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_11) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "FeatureSelector", {"5", "7", "8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "retval", {"11", "16"}));
  doAnalysisAndCompareResults("basic_11_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "retval", {"8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "i", {"9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "j", {"12", "9", "10", "11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 14, "k", {"15", "1", "2", "13", "12", "9", "10", "11"}));
  doAnalysisAndCompareResults("call_01_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "retval", {"12"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "i", {"13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "j", {"14"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 13, "k",
          {"4", "5", "15", "6", "3", "14", "2", "13", "16", "18"}));
  doAnalysisAndCompareResults("call_02_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "retval", {"20"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "i", {"21"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "j",
          {"22", "15", "6", "21", "2", "13", "8", "9", "12", "10", "24"}));
  doAnalysisAndCompareResults("call_03_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "retval", {"33"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "i", {"34"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "j",
          {"15", "6", "2", "13", "8", "9", "12", "10", "35", "34", "37"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 20, "k",
          {"41", "19", "15", "6",  "44", "2",  "13", "8",  "45",
           "18", "9",  "12", "10", "46", "24", "25", "35", "27",
           "23", "26", "38", "34", "37", "42", "40"}));
  doAnalysisAndCompareResults("call_04_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "retval", {"8"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "i", {"3", "11", "9"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 10, "j", {"3", "10", "12"}));
  doAnalysisAndCompareResults("call_05_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_06) {
  // NOTE: Here we are suffering from IntraProceduralAliasesOnly
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "retval", {"11"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "i", {"3", "1", "2", "16", "18", "12"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "j", {"19", "21", "3", "1", "2", "13"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "k", {"22", "3", "14", "1", "2", "24"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 24, "l", {"15", "3", "1", "2", "25", "27"}));
  doAnalysisAndCompareResults("call_06_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_07) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "retval", {"7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "VarIR", {"6", "3", "8"}));
  doAnalysisAndCompareResults("call_07_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "retval", {"3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "i", {"7"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 9, "j", {"0", "5", "6"}));
  doAnalysisAndCompareResults("global_01_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "_Z5initBv", 2, "a", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "_Z5initBv", 2, "b", {"2"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "a", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "b", {"2"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "retval", {"6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "c", {"1", "8", "7"}));
  doAnalysisAndCompareResults("global_02_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 1, "GlobalFeature", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 2, "GlobalFeature", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 17, "GlobalFeature", {"0"}));
  doAnalysisAndCompareResults("global_03_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 1, "GlobalFeature", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 2, "GlobalFeature", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 17, "GlobalFeature", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "_Z7doStuffi", 1, "GlobalFeature", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "_Z7doStuffi", 2, "GlobalFeature", {"0"}));
  doAnalysisAndCompareResults("global_04_cpp.ll", {"main", "_Z7doStuffi"},
                              GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, KillTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "retval", {"4"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "i", {"5"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "j", {"10"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "k", {"9", "8", "5"}));
  doAnalysisAndCompareResults("KillTest_01_cpp.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, KillTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "retval", {"6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "A", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "B", {"2"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 12, "C", {"1", "7", "8"}));
  doAnalysisAndCompareResults("KillTest_02_cpp.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleReturnTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "retval", {"3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "localVar", {"4"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 6, "call", {"0"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 8, "localVar", {"0", "6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 8, "call", {"0"}));
  doAnalysisAndCompareResults("return_01_cpp.ll", {"main"}, GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleHeapTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 17, "retval", {"3"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 17, "i", {"5", "6"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 17, "j", {"5", "6", "7", "8", "9"}));
  doAnalysisAndCompareResults("heap_01_cpp.ll", {"main"}, GroundTruth, false);
}

PHASAR_SKIP_TEST(TEST_F(IDEInstInteractionAnalysisTest, HandleRVOTest_01) {
  GTEST_SKIP() << "This test heavily depends on the used stdlib version. TODO: "
                  "add a better one";

  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 16, "retval", {"75", "76"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 16, "str", {"70", "65", "72", "74", "77"}));
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string, BitVectorSet<std::string>>(
          "main", 16, "ref.tmp", {"66", "9", "72", "73", "71"}));
  doAnalysisAndCompareResults("rvo_01_cpp.ll", {"main"}, GroundTruth, false);
})

// TEST_F(IDEInstInteractionAnalysisTest, HandleStruct_01) {
//   std::set<IIACompactResult_t> GroundTruth;
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//       BitVectorSet<std::string>>(
//           "main", 10, "retval", {"3"}));
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//       BitVectorSet<std::string>>(
//           "main", 10, "a", {"1", "4", "5", "6", "7", "8", "13"}));
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//       BitVectorSet<std::string>>(
//           "main", 10, "x", {"1", "4", "5", "13"}));
//   doAnalysisAndCompareResults("struct_01_cpp.ll", GroundTruth, false);
// }

// main function for the test case/*  */
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
