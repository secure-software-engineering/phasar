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
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/HelperAnalysisConfig.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>

using namespace psr;
using namespace psr::unittest;

using TaintSetT = BitVectorSet<TestingSrcLocation>;

/* ============== TEST FIXTURE ============== */
class IDEInstInteractionAnalysisTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("inst_interaction/");

  using VarNameT = std::variant<std::string, unittest::RetVal>;
  // Function - Line Nr - Variable - Values
  using IIACompactResult_t =
      std::tuple<TestingSrcLocation, VarNameT,
                 IDEInstInteractionAnalysisT<TestingSrcLocation>::l_t>;

  std::optional<HelperAnalyses> HA;
  LLVMProjectIRDB *IRDB{};

  void initializeIR(const std::string &LlvmFilePath,
                    const std::vector<std::string> &EntryPoints = {"main"}) {
    HA.emplace(PathToLlFiles + LlvmFilePath, EntryPoints,
               HelperAnalysisConfig{}.withCGType(CallGraphAnalysisType::CHA));
    IRDB = &HA->getProjectIRDB();
  }

  [[nodiscard]] const llvm::Instruction *getInst(TestingSrcLocation Loc) {
    const auto *Ret = llvm::dyn_cast_if_present<llvm::Instruction>(
        testingLocInIR(Loc, HA->getProjectIRDB()));
    if (!Ret) {
      throw std::runtime_error("Cannot convert loc " + Loc.str() + " to LLVM");
    }
    return Ret;
  }

  [[nodiscard]] bool matchesVar(const llvm::Value *Fact,
                                const VarNameT &VarName) {
    return std::visit(
        psr::Overloaded{
            [&](const std::string &Name) {
              if (!llvm::isa<llvm::AllocaInst>(Fact) &&
                  !llvm::isa<llvm::GlobalVariable>(Fact)) {
                return false;
              }
              auto FactName = psr::getVarNameFromIR(Fact);
              return FactName == Name;
            },
            [&](RetVal R) {
              return llvm::any_of(Fact->users(), [R](const auto *V) {
                const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(V);
                return Ret && Ret->getFunction()->getName() == R.InFunction;
              });
            },
        },
        VarName);
  }
  [[nodiscard]] std::string printVar(const VarNameT &VarName) {
    return std::visit(psr::Overloaded{
                          [](const std::string &Name) { return Name; },
                          [](RetVal R) { return R.str(); },
                      },
                      VarName);
  }
  [[nodiscard]] LatticeDomain<std::set<TestingSrcLocation>>
  sorted(const IDEInstInteractionAnalysisT<TestingSrcLocation>::l_t &Values) {
    if (const auto *Set = Values.getValueOrNull()) {
      std::set<TestingSrcLocation> Ret(Set->begin(), Set->end());
      return Ret;
    }
    if (Values.isBottom()) {
      return Bottom{};
    }
    return Top{};
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

    assert(HA);
    auto IIAProblem =
        createAnalysisProblem<IDEInstInteractionAnalysisT<TestingSrcLocation>>(
            *HA, EntryPoints);

    auto Generator =
        [](std::variant<const llvm::Instruction *, const llvm::GlobalVariable *>
               Current) -> std::set<TestingSrcLocation> {
      return std::visit(
          psr::Overloaded{
              [](const llvm::GlobalVariable *Glob)
                  -> std::set<TestingSrcLocation> {
                std::set<TestingSrcLocation> Labels;
                Labels.insert(GlobalVar{Glob->getName()});
                return Labels;
              },
              [](const llvm::Instruction *Inst)
                  -> std::set<TestingSrcLocation> {
                std::set<TestingSrcLocation> Labels;
                auto [Line, Col] = getLineAndColFromIR(Inst);
                if (Col == 0 && llvm::isa<llvm::StoreInst>(Inst)) {
                  std::tie(Line, Col) = getLineAndColFromIR(Inst->getOperand(
                      llvm::StoreInst::getPointerOperandIndex()));
                }
                if (Line != 0) {
                  Labels.insert(LineColFun{
                      Line,
                      Col,
                      Inst->getFunction()->getName(),
                  });
                }
                return Labels;
              }},
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
    for (const auto &[InstLoc, VarName, ExpectedVal] : GroundTruth) {
      //   const auto *Fun = IRDB->getFunctionDefinition(FunName);
      //   const auto *IRLine = getNthInstruction(Fun, SrcLine);
      const auto *IRLoc = testingLocInIR(InstLoc, *IRDB);
      ASSERT_TRUE(IRLoc) << "Could not retrieve IR Loc: " << InstLoc.str();
      ASSERT_TRUE(llvm::isa<llvm::Instruction>(IRLoc));
      auto ResultMap =
          IIASolver.resultsAt(llvm::cast<llvm::Instruction>(IRLoc));
      bool FactFound = false;
      for (auto &[Fact, ComputedVal] : ResultMap) {
        if (matchesVar(Fact.getBase(), VarName)) {
          EXPECT_EQ(sorted(ExpectedVal), sorted(ComputedVal))
              << "Unexpected taint-set at " << InstLoc << " for variable '"
              << printVar(VarName) << "' (" << llvmIRToString(Fact.getBase())
              << ")";
          FactFound = true;
        }
      }

      EXPECT_TRUE(FactFound)
          << "Variable '" << printVar(VarName) << "' missing at '"
          << llvmIRToString(IRLoc) << "'.";
    }
  }

  void TearDown() override {
    BitVectorSet<TestingSrcLocation>::clearPosition();
  }

}; // Test Fixture

TEST_F(IDEInstInteractionAnalysisTest, FieldSensArrayConstruction_01) {
  initializeIR("array_01_cpp_dbg.ll");
  const auto *Inst = getInst(LineColFun{2, 7, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getInst(LineColFun{5, 3, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getInst(LineColFun{6, 3, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  ASSERT_TRUE(true);
}

TEST_F(IDEInstInteractionAnalysisTest, FieldSensArrayConstruction_02) {
  initializeIR("array_02_cpp_dbg.ll");
  const auto *Inst = getInst(LineColFun{2, 7, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getInst(LineColFun{4, 7, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getInst(LineColFun{3, 3, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getInst(OperandOf{llvm::StoreInst::getPointerOperandIndex(),
                           LineColFun{3, 16, "main"}});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  ASSERT_TRUE(true);
}

TEST_F(IDEInstInteractionAnalysisTest, FieldSensArrayConstruction_03) {
  initializeIR("array_03_cpp_dbg.ll");
  const auto *Inst = getInst(LineColFun{2, 7, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';

  auto Store = LineColFun{3, 19, "main"};
  auto LastGep = OperandOf{llvm::StoreInst::getPointerOperandIndex(), Store};
  auto FirstGep = LineColFun{3, 3, "main"};

  Inst = getInst(FirstGep);
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';

  const auto *LastGepInst = getInst(LastGep);

  Inst = llvm::cast<llvm::Instruction>(LastGepInst->getOperand(0));
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = LastGepInst;
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  ASSERT_TRUE(true);
}

TEST_F(IDEInstInteractionAnalysisTest, FieldSensStructConstruction_01) {
  initializeIR("struct_01_cpp_dbg.ll");
  const auto *Inst = getInst(LineColFun{8, 7, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getInst(LineColFun{12, 5, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getInst(LineColFun{13, 5, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  ASSERT_TRUE(true);
}

TEST_F(IDEInstInteractionAnalysisTest, FieldSensStructConstruction_02) {
  initializeIR("struct_02_cpp_dbg.ll");
  const auto *Inst = getInst(LineColFun{12, 5, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getInst(LineColFun{13, 5, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getInst(LineColFun{13, 7, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  Inst = getInst(LineColFun{14, 5, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << FlowFact << '\n';
  ASSERT_TRUE(true);
}

TEST_F(IDEInstInteractionAnalysisTest, ArrayEquality_01) {
  initializeIR("array_01_cpp_dbg.ll");

  const auto *Inst = getInst(LineColFun{2, 7, "main"});
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, FlowFact);

  Inst = getInst(LineColFun{4, 7, "main"});
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{5, 3, "main"});
  auto OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_NE(FlowFact, OtherFlowFact);

  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{7, 11, "main"});
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getInst(LineColFun{6, 3, "main"});
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{8, 11, "main"});
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);
}

TEST_F(IDEInstInteractionAnalysisTest, ArrayEquality_02) {
  initializeIR("array_02_cpp_dbg.ll");
  const auto *Inst = getInst(LineColFun{2, 7, "main"});
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, FlowFact);

  const auto *FirstGep = getInst(LineColFun{3, 3, "main"});
  Inst = FirstGep;
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{4, 11, "main"});
  auto OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  const auto *SecondGep = getInst(OperandOf{
      llvm::StoreInst::getPointerOperandIndex(), LineColFun{3, 16, "main"}});
  Inst = SecondGep;
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = llvm::cast<llvm::Instruction>(
      getInst(LineColFunOp{4, 11, "main", llvm::Instruction::Load})
          ->getOperand(0));
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = FirstGep;
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = SecondGep;
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_NE(FlowFact, OtherFlowFact);
}

TEST_F(IDEInstInteractionAnalysisTest, ArrayEquality_03) {
  initializeIR("array_03_cpp_dbg.ll");
  const auto *Inst = getInst(LineColFun{2, 7, "main"});
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, FlowFact);

  Inst = getInst(LineColFun{3, 3, "main"});
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{4, 11, "main"});
  auto OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  const auto *GepStore =
      llvm::cast<llvm::StoreInst>(getInst(LineColFun{3, 19, "main"}));
  const auto *GepLoad =
      getInst(LineColFunOp{4, 11, "main", llvm::Instruction::Load});

  Inst = llvm::cast<llvm::Instruction>(
      llvm::cast<llvm::GetElementPtrInst>(GepStore->getPointerOperand())
          ->getPointerOperand());
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = llvm::cast<llvm::Instruction>(
      llvm::cast<llvm::GetElementPtrInst>(GepLoad->getOperand(0))
          ->getPointerOperand());
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = llvm::cast<llvm::Instruction>(GepStore->getPointerOperand());
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = llvm::cast<llvm::Instruction>(GepLoad->getOperand(0));
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = llvm::cast<llvm::Instruction>(
      llvm::cast<llvm::GetElementPtrInst>(GepStore->getPointerOperand())
          ->getPointerOperand());
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = llvm::cast<llvm::Instruction>(GepLoad->getOperand(0));
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  // For K-limit of 2, this should be considered equal
  if (IDEIIAFlowFact::KLimit <= 2) {
    ASSERT_EQ(FlowFact, OtherFlowFact);
  } else {
    ASSERT_NE(FlowFact, OtherFlowFact);
  }
}

TEST_F(IDEInstInteractionAnalysisTest, StructEquality_01) {
  initializeIR("struct_01_cpp_dbg.ll");
  const auto *Inst = getInst(LineColFun{8, 7, "main"});
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, FlowFact);

  Inst = getInst(LineColFun{12, 5, "main"});
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{15, 13, "main"});
  auto OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getInst(LineColFun{13, 5, "main"});
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{16, 13, "main"});
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getInst(LineColFun{14, 5, "main"});
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{17, 13, "main"});
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getInst(LineColFun{13, 5, "main"});
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{14, 5, "main"});
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  llvm::outs() << "Compare:\n";
  llvm::outs() << FlowFact << '\n';
  llvm::outs() << OtherFlowFact << '\n';
  // FIXME
  // ASSERT_NE(FlowFact, OtherFlowFact);
}

TEST_F(IDEInstInteractionAnalysisTest, StructEquality_02) {
  initializeIR("struct_02_cpp_dbg.ll");
  const auto *Inst = getInst(LineColFun{12, 5, "main"});
  auto FlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, FlowFact);

  Inst = getInst(LineColFun{13, 5, "main"});
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{15, 13, "main"});
  auto OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getInst(LineColFun{13, 5, "main"});
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{13, 7, "main"});
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_NE(FlowFact, OtherFlowFact);

  Inst = getInst(LineColFun{13, 7, "main"});
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{15, 15, "main"});
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getInst(LineColFun{14, 5, "main"});
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{16, 13, "main"});
  OtherFlowFact = IDEIIAFlowFact::create(Inst);
  ASSERT_EQ(FlowFact, OtherFlowFact);

  Inst = getInst(LineColFun{13, 5, "main"});
  llvm::outs() << "Instruction to create flow fact from: " << *Inst << '\n';
  FlowFact = IDEIIAFlowFact::create(Inst);
  Inst = getInst(LineColFun{14, 5, "main"});
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
  auto Main9 = LineColFun{4, 3, "main"};
  GroundTruth.emplace(Main9, "i", TaintSetT{LineColFun{2, 7, "main"}});
  GroundTruth.emplace(Main9, "j",
                      TaintSetT{
                          LineColFun{2, 7, "main"},
                          LineColFun{3, 11, "main"},
                          LineColFun{3, 13, "main"},
                          LineColFun{3, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main24 = LineColFun{10, 3, "main"};

  GroundTruth.emplace(Main24, "argc", TaintSetT{LineColFun{1, 14, "main"}});
  GroundTruth.emplace(Main24, "argv", TaintSetT{LineColFun{1, 27, "main"}});
  GroundTruth.emplace(Main24, "i",
                      TaintSetT{
                          LineColFun{5, 7, "main"},
                          LineColFun{7, 7, "main"},
                      });
  GroundTruth.emplace(Main24, "j",
                      TaintSetT{
                          LineColFun{2, 7, "main"},
                          LineColFun{3, 11, "main"},
                          LineColFun{3, 13, "main"},
                          LineColFun{3, 7, "main"},
                      });
  GroundTruth.emplace(Main24, "k",
                      TaintSetT{
                          LineColFun{5, 7, "main"},
                          LineColFun{7, 7, "main"},
                          LineColFun{9, 11, "main"},
                          LineColFun{9, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_02_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_03) {
  std::set<IIACompactResult_t> GroundTruth;

  auto Main20 = LineColFun{6, 3, "main"};

  GroundTruth.emplace(Main20, "i",
                      TaintSetT{
                          LineColFun{2, 7, "main"},
                          LineColFun{4, 5, "main"},
                      });
  GroundTruth.emplace(Main20, "x",
                      TaintSetT{
                          LineColFun{3, 12, "main"},
                          LineColFun{3, 28, "main"},
                      });

  doAnalysisAndCompareResults("basic_03_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_04) {

  LIBCPP_GTEST_SKIP;

  std::set<IIACompactResult_t> GroundTruth;
  auto Main23 = LineColFun{11, 3, "main"};

  GroundTruth.emplace(Main23, "argc",
                      TaintSetT{
                          LineColFun{3, 14, "main"},
                      });
  GroundTruth.emplace(Main23, "argv",
                      TaintSetT{
                          LineColFun{3, 27, "main"},
                      });
  GroundTruth.emplace(Main23, "i",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                      });
  GroundTruth.emplace(Main23, "j",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 11, "main"},
                          LineColFun{5, 13, "main"},
                          LineColFun{5, 7, "main"},
                      });
  GroundTruth.emplace(Main23, "k",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 11, "main"},
                          LineColFun{5, 13, "main"},
                          LineColFun{5, 7, "main"},
                          LineColFun{6, 7, "main"},
                          LineColFun{8, 9, "main"},
                          LineColFun{8, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_04_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main11 = LineColFun{10, 3, "main"};
  GroundTruth.emplace(Main11, "i",
                      TaintSetT{
                          LineColFun{6, 7, "main"},
                          LineColFun{8, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_05_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_06) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main19 = LineColFun{14, 3, "main"};

  GroundTruth.emplace(Main19, "i",
                      TaintSetT{
                          LineColFun{6, 7, "main"},
                          LineColFun{13, 8, "main"},
                          LineColFun{13, 6, "main"},
                      });
  GroundTruth.emplace(Main19, "j",
                      TaintSetT{
                          LineColFun{6, 7, "main"},
                          LineColFun{13, 8, "main"},
                          LineColFun{13, 6, "main"},
                      });
  GroundTruth.emplace(Main19, "k",
                      TaintSetT{
                          LineColFun{6, 7, "main"},
                      });
  GroundTruth.emplace(Main19, "p",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 7, "main"},
                          LineColFun{9, 7, "main"},
                          LineColFun{11, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_06_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_07) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main15 = LineColFun{5, 3, "main"};

  GroundTruth.emplace(Main15, "argc",
                      TaintSetT{
                          LineColFun{1, 14, "main"},
                      });
  GroundTruth.emplace(Main15, "argv",
                      TaintSetT{
                          LineColFun{1, 27, "main"},
                      });
  // strong update on i
  GroundTruth.emplace(Main15, "i",
                      TaintSetT{
                          LineColFun{4, 5, "main"},
                      });
  GroundTruth.emplace(Main15, "j",
                      TaintSetT{
                          LineColFun{2, 7, "main"},
                          LineColFun{3, 11, "main"},
                          LineColFun{3, 13, "main"},
                          LineColFun{3, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_07_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_08) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main12 = LineColFun{11, 3, "main"};

  // strong update on i
  GroundTruth.emplace(Main12, "i",
                      TaintSetT{
                          LineColFun{10, 5, "main"},
                      });

  doAnalysisAndCompareResults("basic_08_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_09) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main10 = LineColFun{6, 3, "main"};

  GroundTruth.emplace(Main10, "i",
                      TaintSetT{
                          LineColFun{3, 7, "main"},
                      });
  GroundTruth.emplace(Main10, "j",
                      TaintSetT{
                          LineColFun{3, 7, "main"},
                          LineColFun{5, 7, "main"},
                          LineColFun{5, 5, "main"},
                      });

  doAnalysisAndCompareResults("basic_09_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_10) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main6 = LineColFun{4, 3, "main"};
  GroundTruth.emplace(Main6, "i",
                      TaintSetT{
                          LineColFun{3, 7, "main"},
                      });

  doAnalysisAndCompareResults("basic_10_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleBasicTest_11) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main20 = RetStmt{"main"};

  GroundTruth.emplace(Main20, "FeatureSelector",
                      TaintSetT{
                          LineColFun{3, 14, "main"},
                          LineColFun{4, 25, "main"},
                          LineColFun{4, 7, "main"},
                      });

  GroundTruth.emplace(Main20, RetVal{"main"},
                      TaintSetT{
                          LineColFun{7, 5, "main"},
                          LineColFun{15, 3, "main"},
                          LineColFun{16, 1, "main"},
                      });

  doAnalysisAndCompareResults("basic_11_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main14 = RetStmt{"main"};

  GroundTruth.emplace(Main14, "i",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                      });
  GroundTruth.emplace(Main14, "j",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 11, "main"},
                          LineColFun{5, 13, "main"},
                          LineColFun{5, 7, "main"},
                      });
  GroundTruth.emplace(Main14, "k",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 11, "main"},
                          LineColFun{5, 13, "main"},
                          LineColFun{5, 7, "main"},
                          LineColFun{6, 7, "main"},
                          LineColFun{6, 14, "main"},
                          LineColFun{1, 12, "_Z2idi"},
                          LineColFun{1, 24, "_Z2idi"},
                      });

  doAnalysisAndCompareResults("call_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main13 = RetStmt{"main"};

  GroundTruth.emplace(Main13, "i",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                      });
  GroundTruth.emplace(Main13, "j",
                      TaintSetT{
                          LineColFun{5, 7, "main"},
                      });
  GroundTruth.emplace(Main13, "k",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{5, 7, "main"},
                          LineColFun{6, 15, "main"},
                          LineColFun{6, 18, "main"},
                          LineColFun{6, 7, "main"},
                          LineColFun{1, 13, "_Z3sumii"},
                          LineColFun{1, 20, "_Z3sumii"},
                          LineColFun{1, 32, "_Z3sumii"},
                          LineColFun{1, 36, "_Z3sumii"},
                          LineColFun{1, 34, "_Z3sumii"},
                      });

  doAnalysisAndCompareResults("call_02_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main10 = RetStmt{"main"};

  GroundTruth.emplace(Main10, "i",
                      TaintSetT{
                          LineColFun{9, 7, "main"},
                      });
  GroundTruth.emplace(Main10, "j",
                      TaintSetT{
                          LineColFun{9, 7, "main"},
                          LineColFun{10, 21, "main"},
                          LineColFun{6, 1, "_Z9factorialj"},
                          LineColFun{3, 5, "_Z9factorialj"},
                          LineColFun{9, 7, "main"},
                          LineColFun{1, 29, "_Z9factorialj"},
                          LineColFun{5, 3, "_Z9factorialj"},
                          LineColFun{5, 10, "_Z9factorialj"},
                          LineColFun{5, 24, "_Z9factorialj"},
                          LineColFun{5, 12, "_Z9factorialj"},
                          LineColFun{5, 26, "_Z9factorialj"},
                          LineColFun{10, 7, "main"},
                      });

  doAnalysisAndCompareResults("call_03_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main10 = RetStmt{"main"};

  GroundTruth.emplace(Main10, "i",
                      TaintSetT{
                          LineColFun{13, 7, "main"},
                      });
  GroundTruth.emplace(Main10, "j",
                      TaintSetT{
                          LineColFun{6, 1, "_Z9factorialj"},
                          LineColFun{3, 5, "_Z9factorialj"},
                          LineColFun{1, 29, "_Z9factorialj"},
                          LineColFun{5, 3, "_Z9factorialj"},
                          LineColFun{5, 10, "_Z9factorialj"},
                          LineColFun{5, 24, "_Z9factorialj"},
                          LineColFun{5, 12, "_Z9factorialj"},
                          LineColFun{5, 26, "_Z9factorialj"},
                          LineColFun{14, 21, "main"},
                          LineColFun{13, 7, "main"},
                          LineColFun{14, 7, "main"},
                      });
  GroundTruth.emplace(Main10, "k",
                      TaintSetT{
                          LineColFun{16, 12, "main"},
                          LineColFun{8, 24, "_Z2idi"},
                          LineColFun{6, 1, "_Z9factorialj"},
                          LineColFun{3, 5, "_Z9factorialj"},
                          LineColFun{16, 5, "main"},
                          LineColFun{1, 29, "_Z9factorialj"},
                          LineColFun{5, 3, "_Z9factorialj"},
                          LineColFun{5, 10, "_Z9factorialj"},
                          LineColFun{16, 5, "main"},
                          LineColFun{8, 12, "_Z2idi"},
                          LineColFun{5, 24, "_Z9factorialj"},
                          LineColFun{5, 12, "_Z9factorialj"},
                          LineColFun{5, 26, "_Z9factorialj"},
                          LineColFun{16, 5, "main"},
                          LineColFun{10, 20, "_Z3sumii"},
                          LineColFun{10, 32, "_Z3sumii"},
                          LineColFun{14, 21, "main"},
                          LineColFun{10, 34, "_Z3sumii"},
                          LineColFun{10, 36, "_Z3sumii"},
                          LineColFun{10, 13, "_Z3sumii"},
                          LineColFun{15, 14, "main"},
                          LineColFun{13, 7, "main"},
                          LineColFun{14, 7, "main"},
                          LineColFun{16, 15, "main"},
                          LineColFun{15, 7, "main"},
                      });

  doAnalysisAndCompareResults("call_04_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_05) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main10 = RetStmt{"main"};

  GroundTruth.emplace(Main10, "i",
                      TaintSetT{
                          LineColFun{2, 38, "_Z18setValueToFortyTwoPi"},
                          LineColFun{7, 3, "main"},
                          LineColFun{5, 7, "main"},
                      });
  GroundTruth.emplace(Main10, "j",
                      TaintSetT{
                          LineColFun{2, 38, "_Z18setValueToFortyTwoPi"},
                          LineColFun{6, 7, "main"},
                          LineColFun{8, 3, "main"},
                      });

  doAnalysisAndCompareResults("call_05_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_06) {
  // NOTE: Here we are suffering from IntraProceduralAliasesOnly
  std::set<IIACompactResult_t> GroundTruth;
  auto Main24 = RetStmt{"main"};

  GroundTruth.emplace(Main24, "i",
                      TaintSetT{
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{2, 19, "_Z9incrementi"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{9, 17, "main"},
                          LineColFun{9, 5, "main"},
                          LineColFun{5, 7, "main"},
                      });
  GroundTruth.emplace(Main24, "j",
                      TaintSetT{
                          LineColFun{10, 17, "main"},
                          LineColFun{10, 5, "main"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{2, 19, "_Z9incrementi"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{6, 7, "main"},
                      });
  GroundTruth.emplace(Main24, "k",
                      TaintSetT{
                          LineColFun{11, 17, "main"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{7, 7, "main"},
                          LineColFun{2, 19, "_Z9incrementi"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{11, 5, "main"},
                      });
  GroundTruth.emplace(Main24, "l",
                      TaintSetT{
                          LineColFun{8, 7, "main"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{2, 19, "_Z9incrementi"},
                          LineColFun{2, 31, "_Z9incrementi"},
                          LineColFun{12, 17, "main"},
                          LineColFun{12, 5, "main"},
                      });

  doAnalysisAndCompareResults("call_06_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleCallTest_07) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main6 = RetStmt{"main"};

  GroundTruth.emplace(Main6, "VarIR",
                      TaintSetT{
                          LineColFun{7, 7, "main"},
                          LineColFun{3, 6, "_Z13inputRefParamRi"},
                          LineColFun{8, 3, "main"},
                      });
  doAnalysisAndCompareResults("call_07_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main9 = RetStmt{"main"};

  GroundTruth.emplace(Main9, "i",
                      TaintSetT{
                          LineColFun{6, 5, "main"},
                      });
  GroundTruth.emplace(Main9, "j",
                      TaintSetT{
                          GlobalVar{"i"},
                          LineColFun{5, 7, "main"},
                          LineColFun{5, 5, "main"},
                      });

  doAnalysisAndCompareResults("global_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_02) {
  std::set<IIACompactResult_t> GroundTruth;

  auto Main12 = RetStmt{"main"};
  auto Init2 = RetStmt{"_Z5initBv"};

  GroundTruth.emplace(Init2, "a",
                      TaintSetT{
                          GlobalVar{"a"},
                      });
  GroundTruth.emplace(Init2, "b",
                      TaintSetT{
                          LineColFun{4, 18, "_Z5initBv"},
                      });

  GroundTruth.emplace(Main12, "a",
                      TaintSetT{
                          GlobalVar{"a"},
                      });
  GroundTruth.emplace(Main12, "b",
                      TaintSetT{
                          LineColFun{4, 18, "_Z5initBv"},
                      });
  GroundTruth.emplace(Main12, "c",
                      TaintSetT{
                          GlobalVar{"b"},
                          LineColFun{7, 7, "main"},
                          LineColFun{7, 11, "main"},
                      });

  doAnalysisAndCompareResults("global_02_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_03) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(LineColFun{6, 11, "main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(LineColFun{6, 25, "main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(RetStmt{"main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });

  doAnalysisAndCompareResults("global_03_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleGlobalTest_04) {
  std::set<IIACompactResult_t> GroundTruth;
  GroundTruth.emplace(LineColFun{8, 11, "main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(LineColFun{8, 25, "main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(RetStmt{"main"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(LineColFun{3, 31, "_Z7doStuffi"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });
  GroundTruth.emplace(LineColFun{3, 22, "_Z7doStuffi"}, "GlobalFeature",
                      TaintSetT{
                          GlobalVar{"GlobalFeature"},
                      });

  doAnalysisAndCompareResults("global_04_cpp_dbg.ll", {"main", "_Z7doStuffi"},
                              GroundTruth, false);
}

TEST_F(IDEInstInteractionAnalysisTest, KillTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main12 = RetStmt{"main"};

  GroundTruth.emplace(Main12, "i",
                      TaintSetT{
                          LineColFun{2, 7, "main"},
                      });
  GroundTruth.emplace(Main12, "j",
                      TaintSetT{
                          LineColFun{5, 5, "main"},
                      });
  GroundTruth.emplace(Main12, "k",
                      TaintSetT{
                          LineColFun{4, 7, "main"},
                          LineColFun{4, 11, "main"},
                          LineColFun{2, 7, "main"},
                      });

  doAnalysisAndCompareResults("KillTest_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, KillTest_02) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main12 = RetStmt{"main"};

  GroundTruth.emplace(Main12, "A",
                      TaintSetT{
                          GlobalVar{"A"},
                      });
  GroundTruth.emplace(Main12, "B",
                      TaintSetT{
                          LineColFun{4, 18, "_Z5initBv"},
                      });
  GroundTruth.emplace(Main12, "C",
                      TaintSetT{
                          GlobalVar{"B"},
                          LineColFun{7, 11, "main"},
                          LineColFun{7, 7, "main"},
                      });

  doAnalysisAndCompareResults("KillTest_02_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleReturnTest_01) {
  std::set<IIACompactResult_t> GroundTruth;
  auto Main6 = LineColFun{7, 12, "main"};
  auto Main8 = RetStmt{"main"};

  GroundTruth.emplace(Main6, "localVar",
                      TaintSetT{
                          LineColFun{6, 12, "main"},
                      });
  GroundTruth.emplace(Main8, "localVar",
                      TaintSetT{
                          LineColFun{2, 30, "_Z20returnIntegerLiteralv"},
                          LineColFun{7, 12, "main"},
                      });

  doAnalysisAndCompareResults("return_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

TEST_F(IDEInstInteractionAnalysisTest, HandleHeapTest_01) {
  std::set<IIACompactResult_t> GroundTruth;

  auto Main17 = RetStmt{"main"};
  GroundTruth.emplace(Main17, "i",
                      TaintSetT{
                          LineColFun{3, 12, "main"},
                          LineColFun{3, 8, "main"},
                      });
  GroundTruth.emplace(Main17, "j",
                      TaintSetT{
                          LineColFun{3, 12, "main"},
                          LineColFun{3, 8, "main"},
                          LineColFun{4, 12, "main"},
                          LineColFun{4, 11, "main"},
                          LineColFun{4, 7, "main"},
                      });

  doAnalysisAndCompareResults("heap_01_cpp_dbg.ll", {"main"}, GroundTruth,
                              false);
}

// PHASAR_SKIP_TEST(TEST_F(IDEInstInteractionAnalysisTest, HandleRVOTest_01) {
//   GTEST_SKIP() << "This test heavily depends on the used stdlib version.
//   TODO: "
//                   "add a better one";

//   std::set<IIACompactResult_t> GroundTruth;
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//       BitVectorSet<std::string>>(
//           "main", 16, "retval", {"75", "76"}));
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//       BitVectorSet<std::string>>(
//           "main", 16, "str", {"70", "65", "72", "74", "77"}));
//   GroundTruth.emplace(
//       std::tuple<std::string, size_t, std::string,
//       BitVectorSet<std::string>>(
//           "main", 16, "ref.tmp", {"66", "9", "72", "73", "71"}));
//   doAnalysisAndCompareResults("rvo_01_cpp.ll", {"main"}, GroundTruth, false);
// })

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
