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
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"

#include "SourceMapping.h"
#include "TestConfig.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <sys/types.h>

using namespace std;
using namespace psr;
using json = nlohmann::json;

using CallBackPairTy = std::pair<IDEExtendedTaintAnalysis<>::config_callback_t,
                                 IDEExtendedTaintAnalysis<>::config_callback_t>;

struct SrcCodeLocEntry {
  SrcCodeLocEntry(u_int32_t Line, std::vector<u_int32_t> Column)
      : Line(Line), Column(std::move(Column)) {}
  u_int32_t Line{};
  std::vector<u_int32_t> Column;
  bool operator==(const SrcCodeLocEntry &Other) const {
    return Line == Other.Line && Column == Other.Column;
  }
};

// /* ============== TEST FIXTURE ============== */

class IDETaintAnalysisTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles = PHASAR_BUILD_SUBFOLDER("xtaint/");
  const std::vector<std::string> EntryPoints = {"main"};

  IDETaintAnalysisTest() = default;
  ~IDETaintAnalysisTest() override = default;

  void doAnalysis(
      HelperAnalyses &HA, const std::vector<SrcCodeLocEntry> &GroundTruth,
      std::variant<std::monostate, TaintConfigData *, CallBackPairTy> Config,
      bool DumpResults = true) {

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

    compareResults(TaintProblem, Solver, GroundTruth);
  }

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {}

  void compareResults(IDEExtendedTaintAnalysis<> &TaintProblem,
                      IDESolver_P<IDEExtendedTaintAnalysis<>> &Solver,
                      const std::vector<SrcCodeLocEntry> &GroundTruth) {
    std::vector<SrcCodeLocEntry> FoundLeaks;
    FoundLeaks.reserve(
        TaintProblem.getAllLeaks(Solver.getSolverResults()).size());

    for (const auto &Leak :
         TaintProblem.getAllLeaks(Solver.getSolverResults())) {
      llvm::errs() << "Leak: " << PrettyPrinter{Leak} << '\n';
      u_int32_t SinkId = Leak.first->getDebugLoc()->getLine();
      std::vector<u_int32_t> LeakedValueIds;
      LeakedValueIds.reserve(Leak.second.size());

      for (const auto &LV : Leak.second) {
        if (const auto &Instr = llvm::dyn_cast_or_null<llvm::Instruction>(LV)) {
          if (Instr->getDebugLoc()) {
            LeakedValueIds.emplace_back(Instr->getDebugLoc()->getColumn());
          } else {
            // getDebugLoc was null
            llvm::outs() << "*Instr:\n";
            llvm::outs() << *Instr << "\n";
            ASSERT_TRUE(false);
          }
        } else {
          // Instr was null
          llvm::outs() << "LV Value: " << LV << "\n";
          ASSERT_TRUE(false);
        }
      }

      FoundLeaks.emplace_back(SinkId, LeakedValueIds);
    }

    EXPECT_EQ(FoundLeaks, GroundTruth);

    llvm::outs() << "--------------------------------------------:\n";
    for (const auto &Leak : FoundLeaks) {
      llvm::outs() << "Line:\n";
      llvm::outs() << Leak.Line << "\n";
      llvm::outs() << "Columns:\n";
      for (const auto &Vec : Leak.Column) {
        llvm::outs() << Vec << "\n";
      }
    }
    llvm::outs() << "--------------------------------------------:\n";
  }
}; // Test Fixture

// TODO:
#if false
TEST_F(IDETaintAnalysisTest, XTaint01_Json) {
  map<int, set<string>> Gt;

  Gt[7] = {"6"};

  TaintConfigData Config;

  FunctionData FuncDataMain;
  FuncDataMain.Name = "main";
  FuncDataMain.SourceValues.push_back(0);

  FunctionData FuncDataPrint;
  FuncDataPrint.Name = "_Z5printi";
  FuncDataPrint.SinkValues.push_back(0);

  Config.Functions.push_back(std::move(FuncDataMain));
  Config.Functions.push_back(std::move(FuncDataPrint));

  HelperAnalyses HA({PathToLLFiles + "xtaint01_json_cpp_dbg.ll"}, EntryPoints);

  doAnalysis(HA, Gt, &Config);
}
#endif

TEST_F(IDETaintAnalysisTest, XTaint01) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint01_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(8, {9}));

  doAnalysis(HA, GroundTruth, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint02) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint02_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(9, {9}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint03) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint03_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(10, {9}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint04) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint04_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(6, {9}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

// XTaint05 is similar to 06, but even harder

TEST_F(IDETaintAnalysisTest, XTaint06) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  HelperAnalyses HA({PathToLLFiles + "xtaint06_cpp_dbg.ll"}, EntryPoints);

  // no leaks expected

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

/// In the new TaintConfig specifying source/sink/sanitizer properties for extra
/// parameters of C-style variadic functions is not (yet?) supported. So, the
/// tests XTaint07 and XTaint08 are disabled.
TEST_F(IDETaintAnalysisTest, DISABLED_XTaint07) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint07_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(10, {18}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint08) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint08_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(20, {18}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint09_1) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint09_1_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(14, {8}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint09) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint09_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(16, {8}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint10) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint10_cpp_dbg.ll"}, EntryPoints);

  // GroundTruth.emplace_back(SrcCodeLocEntry(, {}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint11) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint11_cpp_dbg.ll"}, EntryPoints);

  // GroundTruth.emplace_back(SrcCodeLocEntry(, {}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint12) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint12_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(19, {8}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint13) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint13_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(17, {8}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint14) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint14_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(24, {8}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

/// The TaintConfig fails to get all call-sites of Source::get, because it has
/// no CallGraph information
TEST_F(IDETaintAnalysisTest, DISABLED_XTaint15) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint15_cpp_dbg.ll"}, EntryPoints);

  // GroundTruth.emplace_back(SrcCodeLocEntry(, {}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint16) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint16_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(13, {8}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint17) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint17_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(17, {8}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint18) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint18_cpp_dbg.ll"}, EntryPoints);

  // TODO: ask Fabian why there are no leaks found for this test
  // I added a random GT, so that the test fails and it won't be overlooked.
  GroundTruth.emplace_back(SrcCodeLocEntry(100000, {100000}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

PHASAR_SKIP_TEST(TEST_F(IDETaintAnalysisTest, XTaint19) {
  // Is now the same as XTaint17
  GTEST_SKIP();

  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint19_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(17, {8}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
})

TEST_F(IDETaintAnalysisTest, XTaint20) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(2);
  HelperAnalyses HA({PathToLLFiles + "xtaint20_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(17, {8}));

  doAnalysis(HA, GroundTruth, std::monostate{}, true);
}

TEST_F(IDETaintAnalysisTest, XTaint21) {
  std::vector<SrcCodeLocEntry> GroundTruth;
  GroundTruth.reserve(1);
  HelperAnalyses HA({PathToLLFiles + "xtaint21_cpp_dbg.ll"}, EntryPoints);

  GroundTruth.emplace_back(SrcCodeLocEntry(17, {8}));

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
