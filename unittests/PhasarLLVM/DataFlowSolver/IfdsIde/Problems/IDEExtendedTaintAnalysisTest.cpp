/******************************************************************************
 * Copyright (c) 2021 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "gtest/gtest.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"

#include "nlohmann/json.hpp"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/DebugOutput.h"
#include "phasar/Utils/Utilities.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;
using json = nlohmann::json;

using CallBackPairTy = std::pair<IDEExtendedTaintAnalysis<>::config_callback_t,
                                 IDEExtendedTaintAnalysis<>::config_callback_t>;

// /* ============== TEST FIXTURE ============== */

class IDETaintAnalysisTest : public ::testing::Test {
protected:
  const std::string PathToLLFiles = unittest::PathToLLTestFiles + "xtaint/";
  const std::set<std::string> EntryPoints = {"main"};

  IDETaintAnalysisTest() = default;
  ~IDETaintAnalysisTest() override = default;

  void doAnalysis(const std::vector<std::string> &IRFiles,
                  const map<int, set<string>> &GroundTruth,
                  std::variant<std::monostate, json *, CallBackPairTy> Config,
                  bool DumpResults = false) {
    ProjectIRDB IRDB(IRFiles, IRDBOptions::WPA);

    LLVMTypeHierarchy TH(IRDB);
    // llvm::errs() << "TH: " << TH << '\n';
    LLVMPointsToSet PT(IRDB);
    LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, EntryPoints, &TH, &PT);
    auto TC =
        std::visit(Overloaded{[&](std::monostate) { return TaintConfig(IRDB); },
                              [&](json *JS) {
                                auto Ret = TaintConfig(IRDB, *JS);
                                if (DumpResults) {
                                  llvm::errs() << Ret << "\n";
                                }
                                return Ret;
                              },
                              [&](CallBackPairTy &CB) {
                                return TaintConfig(CB.first, CB.second);
                              }},
                   Config);

    IDEExtendedTaintAnalysis<> TaintProblem(&IRDB, &TH, &ICFG, &PT, TC,
                                            EntryPoints);

    IDESolver_P<IDEExtendedTaintAnalysis<>> Solver(TaintProblem);
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
                      const map<int, set<string>> &GroundTruth) {

    map<int, set<string>> FoundLeaks;
    for (const auto &Leak : TaintProblem.getAllLeaks(Solver)) {
      llvm::errs() << "Leak: " << PrettyPrinter{Leak} << '\n';
      int SinkId = stoi(getMetaDataID(Leak.first));
      set<string> LeakedValueIds;
      for (const auto &LV : Leak.second) {
        LeakedValueIds.insert(getMetaDataID(LV));
      }
      FoundLeaks.emplace(SinkId, LeakedValueIds);
    }
    EXPECT_EQ(FoundLeaks, GroundTruth);
  }
}; // Test Fixture

TEST_F(IDETaintAnalysisTest, XTaint01_Json) {
  map<int, set<string>> Gt;

  Gt[7] = {"6"};

  json Config = R"!({
    "name": "XTaintTest",
    "version": 1.0,
    "functions": [
      {
        "file": "xtaint01.cpp",
        "name": "main",
        "params": {
          "source": [
            0
          ]
        }
      },
      {
        "file": "xtaint01.cpp",
        "name": "_Z5printi",
        "params": {
          "sink": [
            0
          ]
        }
      }
    ]
    })!"_json;

  doAnalysis({PathToLLFiles + "xtaint01_json_cpp_dbg.ll"}, Gt, &Config);
}

TEST_F(IDETaintAnalysisTest, XTaint01) {
  map<int, set<string>> Gt;

  Gt[15] = {"14"};

  doAnalysis({PathToLLFiles + "xtaint01_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint02) {
  map<int, set<string>> Gt;

  Gt[20] = {"19"};

  doAnalysis({PathToLLFiles + "xtaint02_cpp.ll"}, Gt, std::monostate{}, true);
}
TEST_F(IDETaintAnalysisTest, XTaint03) {
  map<int, set<string>> Gt;

  Gt[23] = {"22"};

  doAnalysis({PathToLLFiles + "xtaint03_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint04) {
  map<int, set<string>> Gt;

  Gt[17] = {"16"};

  doAnalysis({PathToLLFiles + "xtaint04_cpp.ll"}, Gt, std::monostate{});
}

// XTaint05 is similar to 06, but even harder

TEST_F(IDETaintAnalysisTest, XTaint06) {
  map<int, set<string>> Gt;

  // no leaks expected

  doAnalysis({PathToLLFiles + "xtaint06_cpp.ll"}, Gt, std::monostate{});
}

/// In the new TaintConfig specifying source/sink/sanitizer properties for extra
/// parameters of C-style variadic functions is not (yet?) supported. So, the
/// tests XTaint07 and XTaint08 are disabled.
TEST_F(IDETaintAnalysisTest, DISABLED_XTaint07) {
  map<int, set<string>> Gt;

  Gt[21] = {"20"};

  doAnalysis({PathToLLFiles + "xtaint07_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint08) {
  map<int, set<string>> Gt;

  Gt[24] = {"23"};

  doAnalysis({PathToLLFiles + "xtaint08_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint09_1) {
  map<int, set<string>> Gt;

  Gt[27] = {"26"};

  doAnalysis({PathToLLFiles + "xtaint09_1_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint09) {
  map<int, set<string>> Gt;

  Gt[34] = {"33"};

  doAnalysis({PathToLLFiles + "xtaint09_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint10) {
  map<int, set<string>> Gt;

  // undefined behaviour: sometimes this test fails, but most of the time
  // it passes. It only fails when executed together with other tests. It
  // never failed (so far) for ./IDEExtendedTaintAnalysisTest
  // --Gtest_filter=*XTaint10
  // UPDATE: With the fixed k-limiting, this test
  // almost always fails due to aliasing issues, so disable it.
  // TODO: Also update the Gt
  Gt[33] = {"32"};

  doAnalysis({PathToLLFiles + "xtaint10_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint11) {
  map<int, set<string>> Gt;

  // no leaks expected; actually finds "27" at 28

  doAnalysis({PathToLLFiles + "xtaint11_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint12) {
  map<int, set<string>> Gt;

  // We sanitize an alias - since we don't have must-alias relations, we cannot
  // kill aliases at all
  Gt[30] = {"29"};

  doAnalysis({PathToLLFiles + "xtaint12_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint13) {
  map<int, set<string>> Gt;

  Gt[32] = {"31"};

  doAnalysis({PathToLLFiles + "xtaint13_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint14) {
  map<int, set<string>> Gt;

  Gt[35] = {"34"};

  doAnalysis({PathToLLFiles + "xtaint14_cpp.ll"}, Gt, std::monostate{});
}

/// The TaintConfig fails to get all call-sites of Source::get, because it has
/// no CallGraph information
TEST_F(IDETaintAnalysisTest, DISABLED_XTaint15) {
  map<int, set<string>> Gt;

  Gt[47] = {"46"};

  doAnalysis({PathToLLFiles + "xtaint15_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint16) {
  map<int, set<string>> Gt;

  Gt[26] = {"25"};

  doAnalysis({PathToLLFiles + "xtaint16_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint17) {
  map<int, set<string>> Gt;

  Gt[29] = {"28"};

  doAnalysis({PathToLLFiles + "xtaint17_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint18) {
  map<int, set<string>> Gt;

  // Gt[26] = {"25"};

  doAnalysis({PathToLLFiles + "xtaint18_cpp.ll"}, Gt, std::monostate{});
}

PHASAR_SKIP_TEST(TEST_F(IDETaintAnalysisTest, XTaint19) {
  // Is now the same as XTaint17
  GTEST_SKIP();
  map<int, set<string>> Gt;

  Gt[22] = {"21"};

  doAnalysis({PathToLLFiles + "xtaint19_cpp.ll"}, Gt, std::monostate{});
})

TEST_F(IDETaintAnalysisTest, XTaint20) {
  map<int, set<string>> Gt;

  Gt[25] = {"17"};
  Gt[27] = {"26"};

  doAnalysis({PathToLLFiles + "xtaint20_cpp.ll"}, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint21) {
  map<int, set<string>> Gt;

  Gt[10] = {"2"};
  Gt[12] = {"11"};

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

  doAnalysis({PathToLLFiles + "xtaint21_cpp.ll"}, Gt,
             CallBackPairTy{std::move(SourceCB), std::move(SinkCB)});
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
