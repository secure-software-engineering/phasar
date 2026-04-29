/******************************************************************************
 * Copyright (c) 2021 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"

#include "phasar/DataFlow/IfdsIde/Solver/GenericSolverResults.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <tuple>
#include <utility>
#include <variant>

using namespace psr;
using namespace psr::unittest;

using CallBackPairTy = std::pair<IDEExtendedTaintAnalysis<>::config_callback_t,
                                 IDEExtendedTaintAnalysis<>::config_callback_t>;

// /* ============== TEST FIXTURE ============== */

class IDETaintAnalysisTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles = PHASAR_BUILD_SUBFOLDER("xtaint/");
  const std::vector<std::string> EntryPoints = {"main"};

  using TaintSetT = std::set<TestingSrcLocation>;

  void doAnalysis(
      const llvm::Twine &IRFilePath,
      const std::map<TestingSrcLocation, TaintSetT> &GroundTruth,
      std::variant<std::monostate, TaintConfigData *, CallBackPairTy> Config,
      bool DumpResults = true) {
    HelperAnalyses HA(PathToLLFiles + IRFilePath, EntryPoints);

    auto TC =
        std::visit(Overloaded{[&](std::monostate) {
                                return LLVMTaintConfig(HA.getProjectIRDB());
                              },
                              [&](TaintConfigData *JS) {
                                LLVMTaintConfig Ret =
                                    LLVMTaintConfig(HA.getProjectIRDB(), *JS);
                                if (DumpResults) {
                                  llvm::errs() << Ret << "\n";
                                }
                                return Ret;
                              },
                              [&](CallBackPairTy &&CB) {
                                return LLVMTaintConfig(std::move(CB.first),
                                                       std::move(CB.second));
                              }},
                   std::move(Config));

    auto TaintProblem =
        createAnalysisProblem<IDEExtendedTaintAnalysis<>>(HA, TC, EntryPoints);

    IDESolver Solver(TaintProblem, &HA.getICFG());
    Solver.solve();
    // Solver.printAnnotatedIR();
    if (DumpResults) {
      Solver.dumpResults();
    }

    TaintProblem.emitTextReport(Solver.getSolverResults());

    compareResults(TaintProblem, Solver.getSolverResults(), GroundTruth);
  }

  void
  compareResults(IDEExtendedTaintAnalysis<> &TaintProblem, auto &&SR,
                 const std::map<TestingSrcLocation, TaintSetT> &GroundTruth) {
    auto GroundTruthEntries = convertTestingLocationSetMapInIR(
        GroundTruth, *TaintProblem.getProjectIRDB());

    std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
        FoundLeaks;

    for (const auto &[LeakInst, LeakVals] : TaintProblem.getAllLeaks(SR)) {
      FoundLeaks[LeakInst].insert(LeakVals.begin(), LeakVals.end());
    }

    EXPECT_EQ(FoundLeaks, GroundTruthEntries);
  }
}; // Test Fixture

TEST_F(IDETaintAnalysisTest, XTaint01_Json) {
  TaintConfigData Config;

  FunctionData FuncDataMain;
  FuncDataMain.Name = "main";
  FuncDataMain.SourceValues.push_back(0);

  FunctionData FuncDataPrint;
  FuncDataPrint.Name = "_Z5printi";
  FuncDataPrint.SinkValues.push_back(0);

  Config.Functions.push_back(std::move(FuncDataMain));
  Config.Functions.push_back(std::move(FuncDataPrint));

  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {{
      LineColFun{8, 3, "main"},
      {LineColFunOp{8, 9, "main", llvm::Instruction::Load}},
  }};

  doAnalysis("xtaint01_json_cpp_dbg.ll", GroundTruth, &Config);
}

TEST_F(IDETaintAnalysisTest, XTaint01) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{8, 3, "main"},
       {LineColFunOp{8, 9, "main", llvm::Instruction::Load}}}};

  doAnalysis("xtaint01_cpp_dbg.ll", GroundTruth, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint02) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{9, 3, "main"},
       {LineColFunOp{9, 9, "main", llvm::Instruction::Load}}}};

  doAnalysis("xtaint02_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint03) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{10, 3, "main"},
       {LineColFunOp{10, 9, "main", llvm::Instruction::Load}}}};

  doAnalysis("xtaint03_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint04) {
  auto Call = LineColFun{6, 3, "_Z3barPi"};

  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {Call, {OperandOf{0, Call}}}};

  doAnalysis("xtaint04_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

// XTaint05 is similar to 06, but even harder

TEST_F(IDETaintAnalysisTest, XTaint06) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      // no leaks expected
  };

  doAnalysis("xtaint06_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

/// In the new TaintConfig specifying source/sink/sanitizer properties for
/// extra parameters of C-style variadic functions is not (yet?) supported.
/// So, the tests XTaint07 and XTaint08 are disabled.
TEST_F(IDETaintAnalysisTest, DISABLED_XTaint07) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{10, 0, "main"},
       {LineColFunOp{10, 18, "main", llvm::Instruction::Load}}}};

  doAnalysis("xtaint07_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint08) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{20, 3, "main"},
       {LineColFunOp{20, 18, "main", llvm::Instruction::Load}}}};

  doAnalysis("xtaint08_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint09_1) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{14, 3, "main"}, {LineColFun{14, 8, "main"}}}};

  doAnalysis("xtaint09_1_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint09) {
  auto SinkCall = LineColFun{16, 3, "main"};
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {SinkCall, {OperandOf{0, SinkCall}}}};

  doAnalysis("xtaint09_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint10) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      // no leaks expected
  };

  doAnalysis("xtaint10_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint11) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      // no leaks expected
  };

  doAnalysis("xtaint11_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint12) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{19, 3, "main"}, {LineColFun{19, 8, "main"}}}};

  doAnalysis("xtaint12_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint13) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{17, 3, "main"}, {LineColFun{17, 8, "main"}}}};

  doAnalysis("xtaint13_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint14) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{24, 3, "main"}, {LineColFun{24, 8, "main"}}}};

  doAnalysis("xtaint14_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

/// The TaintConfig fails to get all call-sites of Source::get, because it has
/// no CallGraph information
TEST_F(IDETaintAnalysisTest, DISABLED_XTaint15) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      // no leaks expected
  };

  doAnalysis("xtaint15_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint16) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{13, 3, "main"}, {LineColFun{13, 8, "main"}}}};

  doAnalysis("xtaint16_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint17) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{17, 3, "main"}, {LineColFun{17, 8, "main"}}}};

  doAnalysis("xtaint17_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint18) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      // no leaks expected
  };

  doAnalysis("xtaint18_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

PHASAR_SKIP_TEST(TEST_F(IDETaintAnalysisTest, XTaint19) {
  // Is now the same as XTaint17
  GTEST_SKIP();

  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{17, 3, "main"}, {LineColFun{17, 8, "main"}}}};

  doAnalysis("xtaint19_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
})

TEST_F(IDETaintAnalysisTest, XTaint20) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{12, 3, "main"}, {LineColFun{6, 7, "main"}}},
      {LineColFun{13, 3, "main"}, {LineColFun{13, 8, "main"}}},
  };

  doAnalysis("xtaint20_cpp_dbg.ll", GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint21) {
  std::map<TestingSrcLocation, TaintSetT> GroundTruth = {
      {LineColFun{17, 3, "main"}, {LineColFun{11, 7, "main"}}},
      {LineColFun{18, 3, "main"}, {LineColFun{18, 8, "main"}}},
  };

  IDEExtendedTaintAnalysis<>::config_callback_t SourceCB =
      [](const llvm::Instruction *Inst) {
        std::set<const llvm::Value *> Ret;
        if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
            Call && Call->getCalledFunction() &&
            Call->getCalledFunction()->getName() == "_Z7srcsinkRi") {
          Ret.insert(Call->getArgOperand(0));
        }
        return Ret;
      };
  IDEExtendedTaintAnalysis<>::config_callback_t SinkCB =
      [](const llvm::Instruction *Inst) {
        std::set<const llvm::Value *> Ret;
        if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst);
            Call && Call->getCalledFunction() &&
            (Call->getCalledFunction()->getName() == "_Z7srcsinkRi" ||
             Call->getCalledFunction()->getName() == "_Z4sinki")) {
          Ret.insert(Call->getArgOperand(0));
        }
        return Ret;
      };

  doAnalysis("xtaint21_cpp_dbg.ll", GroundTruth,
             CallBackPairTy{std::move(SourceCB), std::move(SinkCB)});
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
