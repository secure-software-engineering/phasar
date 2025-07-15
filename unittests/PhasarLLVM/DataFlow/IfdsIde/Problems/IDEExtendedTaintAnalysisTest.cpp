/******************************************************************************
 * Copyright (c) 2021 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"

#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/DebugOutput.h"
#include "phasar/Utils/SrcCodeLocationEntry.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"

#include "SourceMapping.h"
#include "TestConfig.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

using namespace std;
using namespace psr;
using json = nlohmann::json;

using CallBackPairTy = std::pair<IDEExtendedTaintAnalysis<>::config_callback_t,
                                 IDEExtendedTaintAnalysis<>::config_callback_t>;

// /* ============== TEST FIXTURE ============== */

class IDETaintAnalysisTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles = PHASAR_BUILD_SUBFOLDER("xtaint/");
  const std::vector<std::string> EntryPoints = {"main"};

  IDETaintAnalysisTest() = default;
  ~IDETaintAnalysisTest() override = default;

  void doAnalysis(
      HelperAnalyses &HA,
      const std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>>
          &GroundTruth,
      std::variant<std::monostate, TaintConfigData *, CallBackPairTy> Config,
      bool DumpResults = true, const llvm::StringRef FuncName = "main") {
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

    compareResults(TaintProblem, Solver, GroundTruth,
                   HA.getProjectIRDB().getFunction(FuncName));
  }

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {}

  void compareResults(
      IDEExtendedTaintAnalysis<> &TaintProblem,
      IDESolver_P<IDEExtendedTaintAnalysis<>> &Solver,
      const std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>>
          &GroundTruth,
      const llvm::Function *Func) {
    auto GroundTruthEntries = getGroundTruthInsts(GroundTruth, Func);

    // Debug stuff
    uint32_t Line = 12;
    std::set<SrcCodeLocationEntry> TestSet;
    for (uint32_t Col = 0; Col < 30; Col++) {
      SrcCodeLocationEntry Curr = {Line, Col};
      TestSet.insert(Curr);
    }
    llvm::outs() << "TestSet Size: " << TestSet.size() << "\n";

    int Counter = 0;

    auto TestInsts = getGroundTruthInsts(TestSet, Func);
    llvm::outs() << "Line: " << Line << "\n";
    llvm::outs() << "TestInsts Size: " << TestInsts.size() << "\n";
    for (const auto *Elem : TestInsts) {
      if (Elem) {
        llvm::outs() << Counter << ": " << *Elem << " \n";
      } else {
        llvm::outs() << Counter << ": Was nullptr \n";
      }
      Counter++;
    }
    // Debug stuff

    std::set<std::tuple<const llvm::Instruction *, const llvm::Value *>>
        FoundLeaks;

    for (const auto &Leak :
         TaintProblem.getAllLeaks(Solver.getSolverResults())) {
      llvm::errs() << "Leak: " << PrettyPrinter{Leak} << '\n';

      for (const auto &LV : Leak.second) {
        FoundLeaks.insert({Leak.first, LV});
      }
    }

    EXPECT_EQ(FoundLeaks, GroundTruthEntries);

    llvm::outs() << "-----------------GroundTruth----------------:\n";
    for (const auto &Leak : GroundTruthEntries) {
      llvm::outs() << "std::get<0>:\n";
      llvm::outs() << *(std::get<0>(Leak)) << "\n";
      llvm::outs() << "std::get<1>:\n";
      llvm::outs() << *(std::get<1>(Leak)) << "\n";
    }
    llvm::outs() << "------------------FoundLeaks----------------:\n";
    for (const auto &Leak : FoundLeaks) {
      llvm::outs() << "std::get<0>:\n";
      llvm::outs() << *(std::get<0>(Leak)) << "\n";
      llvm::outs() << "std::get<1>:\n";
      llvm::outs() << *(std::get<1>(Leak)) << "\n";
    }
    llvm::outs() << "--------------------------------------------:\n";
  }
}; // Test Fixture

TEST_F(IDETaintAnalysisTest, XTaint01_Json) {
  HelperAnalyses HA({PathToLLFiles + "xtaint01_json_cpp_dbg.ll"}, EntryPoints);

  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  TaintConfigData Config;

  FunctionData FuncDataMain;
  FuncDataMain.Name = "main";
  FuncDataMain.SourceValues.push_back(0);

  FunctionData FuncDataPrint;
  FuncDataPrint.Name = "_Z5printi";
  FuncDataPrint.SinkValues.push_back(0);

  Config.Functions.push_back(std::move(FuncDataMain));
  Config.Functions.push_back(std::move(FuncDataPrint));

  GroundTruth.insert({{8, 3}, {8, 9}});

  doAnalysis(HA, GroundTruth, &Config);
}

TEST_F(IDETaintAnalysisTest, XTaint01) {
  HelperAnalyses HA({PathToLLFiles + "xtaint01_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  GroundTruth.insert({{8, 3}, {8, 9}});

  doAnalysis(HA, GroundTruth, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint02) {
  HelperAnalyses HA({PathToLLFiles + "xtaint02_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  SrcCodeLocationEntry Call = SrcCodeLocationEntry(9, 3);
  SrcCodeLocationEntry Leak =
      SrcCodeLocationEntry(9, 9, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });

  GroundTruth.insert({Call, Leak});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint03) {
  HelperAnalyses HA({PathToLLFiles + "xtaint03_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  SrcCodeLocationEntry Call = SrcCodeLocationEntry(10, 3);
  SrcCodeLocationEntry Leak =
      SrcCodeLocationEntry(10, 9, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });

  GroundTruth.insert({Call, Leak});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint04) {
  HelperAnalyses HA({PathToLLFiles + "xtaint04_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  SrcCodeLocationEntry Call = SrcCodeLocationEntry(6, 3);

  // TODO: this counter stuff is not good, but I haven't found a better way to
  // implement this yet
  int Counter = 0;
  SrcCodeLocationEntry Leak = SrcCodeLocationEntry(
      6, 9, [Counter](const llvm::Instruction *Inst) mutable {
        llvm::outs() << "Inst: " << *Inst << "\n";
        Counter++;
        return Counter == 3;
      });

  GroundTruth.insert({Call, Leak});

  doAnalysis(HA, GroundTruth, std::monostate{}, true, "_Z3barPi");
}

// XTaint05 is similar to 06, but even harder

TEST_F(IDETaintAnalysisTest, XTaint06) {
  HelperAnalyses HA({PathToLLFiles + "xtaint06_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  // no leaks expected

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

/// In the new TaintConfig specifying source/sink/sanitizer properties for
/// extra parameters of C-style variadic functions is not (yet?) supported.
/// So, the tests XTaint07 and XTaint08 are disabled.
TEST_F(IDETaintAnalysisTest, DISABLED_XTaint07) {
  HelperAnalyses HA({PathToLLFiles + "xtaint07_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  SrcCodeLocationEntry Call = SrcCodeLocationEntry(10, 0);
  SrcCodeLocationEntry Leak =
      SrcCodeLocationEntry(10, 18, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  GroundTruth.insert({Call, Leak});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint08) {
  HelperAnalyses HA({PathToLLFiles + "xtaint08_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  SrcCodeLocationEntry Call = SrcCodeLocationEntry(20, 0);
  SrcCodeLocationEntry Leak =
      SrcCodeLocationEntry(20, 18, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  GroundTruth.insert({Call, Leak});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint09_1) {
  HelperAnalyses HA({PathToLLFiles + "xtaint09_1_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  SrcCodeLocationEntry Call = SrcCodeLocationEntry(14, 3);
  SrcCodeLocationEntry Leak = SrcCodeLocationEntry(14, 8);
  GroundTruth.insert({Call, Leak});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint09) {
  HelperAnalyses HA({PathToLLFiles + "xtaint09_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  SrcCodeLocationEntry Call = SrcCodeLocationEntry(16, 3);
  // TODO: this counter stuff is not good, but I haven't found a better way to
  // implement this yet
  int Counter = 0;
  SrcCodeLocationEntry Leak = SrcCodeLocationEntry(
      16, 8, [Counter](const llvm::Instruction *Inst) mutable {
        llvm::outs() << "Inst: " << *Inst << "\n";
        Counter++;
        return Counter == 2;
      });
  GroundTruth.insert({Call, Leak});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint10) {
  HelperAnalyses HA({PathToLLFiles + "xtaint10_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  // GroundTruth.insert(SrcCodeLocEntry(, {}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint11) {
  HelperAnalyses HA({PathToLLFiles + "xtaint11_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  // GroundTruth.insert(SrcCodeLocEntry(, {}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint12) {
  HelperAnalyses HA({PathToLLFiles + "xtaint12_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  GroundTruth.insert(
      {SrcCodeLocationEntry(19, 3), SrcCodeLocationEntry(19, 8)});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint13) {
  HelperAnalyses HA({PathToLLFiles + "xtaint13_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  GroundTruth.insert(
      {SrcCodeLocationEntry(17, 3), SrcCodeLocationEntry(17, 8)});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint14) {
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;
  HelperAnalyses HA({PathToLLFiles + "xtaint14_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.insert(
      {SrcCodeLocationEntry(24, 3), SrcCodeLocationEntry(24, 8)});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

/// The TaintConfig fails to get all call-sites of Source::get, because it has
/// no CallGraph information
TEST_F(IDETaintAnalysisTest, DISABLED_XTaint15) {
  HelperAnalyses HA({PathToLLFiles + "xtaint15_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  // GroundTruth.insert(SrcCodeLocEntry(, {}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint16) {
  HelperAnalyses HA({PathToLLFiles + "xtaint16_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  GroundTruth.insert(
      {SrcCodeLocationEntry(13, 3), SrcCodeLocationEntry(13, 8)});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint17) {
  HelperAnalyses HA({PathToLLFiles + "xtaint17_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  GroundTruth.insert(
      {SrcCodeLocationEntry(17, 3), SrcCodeLocationEntry(17, 8)});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint18) {
  HelperAnalyses HA({PathToLLFiles + "xtaint18_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  // TODO: ask Fabian why there are no leaks found for this test
  // I added a random GT, so that the test fails and it won't be overlooked.
  GroundTruth.insert(
      {SrcCodeLocationEntry(8, 10), SrcCodeLocationEntry(8, 14)});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

PHASAR_SKIP_TEST(TEST_F(IDETaintAnalysisTest, XTaint19) {
  // Is now the same as XTaint17
  GTEST_SKIP();

  HelperAnalyses HA({PathToLLFiles + "xtaint19_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  GroundTruth.insert(
      {SrcCodeLocationEntry(17, 3), SrcCodeLocationEntry(17, 8)});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
})

TEST_F(IDETaintAnalysisTest, XTaint20) {
  HelperAnalyses HA({PathToLLFiles + "xtaint20_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  GroundTruth.insert({SrcCodeLocationEntry(12, 3), SrcCodeLocationEntry(6, 7)});
  GroundTruth.insert(
      {SrcCodeLocationEntry(13, 3), SrcCodeLocationEntry(13, 8)});

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint21) {
  HelperAnalyses HA({PathToLLFiles + "xtaint21_cpp_dbg.ll"}, EntryPoints);
  std::set<std::tuple<SrcCodeLocationEntry, SrcCodeLocationEntry>> GroundTruth;

  GroundTruth.insert(
      {SrcCodeLocationEntry(17, 3), SrcCodeLocationEntry(11, 7)});
  GroundTruth.insert(
      {SrcCodeLocationEntry(18, 3), SrcCodeLocationEntry(18, 8)});

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

  doAnalysis(HA, GroundTruth,
             CallBackPairTy{std::move(SourceCB), std::move(SinkCB)});
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
