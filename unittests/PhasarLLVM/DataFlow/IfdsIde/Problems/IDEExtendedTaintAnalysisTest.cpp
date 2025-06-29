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
      HelperAnalyses &HA, const map<int, set<string>> &GroundTruth,
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
                      const map<int, set<string>> &GroundTruth) {

    map<int, set<string>> FoundLeaks;
    for (const auto &Leak :
         TaintProblem.getAllLeaks(Solver.getSolverResults())) {
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

TEST_F(IDETaintAnalysisTest, XTaint01) {
  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint01_cpp_dbg.ll"}, EntryPoints);

  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  //
  const auto *LeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 8, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  ASSERT_TRUE(LeakLoadInst);

  // print(argc);
  const auto *LeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 8, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(LeakCallInst);

  const auto LeakLoadID = getMetaDataID(LeakLoadInst);
  const auto LeakCallID = getMetaDataID(LeakCallInst);

  Gt[stoi(LeakCallID)] = {LeakLoadID};

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint02) {
  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint02_cpp_dbg.ll"}, EntryPoints);

  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  //
  const auto *LeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 9, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  ASSERT_TRUE(LeakLoadInst);

  // print(array[0]);
  const auto *LeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 9, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(LeakCallInst);

  const auto LeakLoadID = getMetaDataID(LeakLoadInst);
  const auto LeakCallID = getMetaDataID(LeakCallInst);

  Gt[stoi(LeakCallID)] = {LeakLoadID};

  doAnalysis(HA, Gt, std::monostate{}, true);
}
TEST_F(IDETaintAnalysisTest, XTaint03) {
  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint03_cpp_dbg.ll"}, EntryPoints);

  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  // %2 = load i32, ptr %arrayidx2, align 4, !dbg !62, !psr.id !64 | ID: 24
  const auto *LeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 10, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  ASSERT_TRUE(LeakLoadInst);

  // print(array[1]);
  const auto *LeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 10, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(LeakCallInst);

  const auto LeakLoadID = getMetaDataID(LeakLoadInst);
  const auto LeakCallID = getMetaDataID(LeakCallInst);

  Gt[stoi(LeakCallID)] = {LeakLoadID};

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint04) {
  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint04_cpp_dbg.ll"}, EntryPoints);

  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("_Z3barPi");

  // %3 = load i32, ptr %arrayidx1, align 4, !dbg !42, !psr.id !45 | ID: 17
  const auto *LeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 6, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst) &&
               Inst->getOperand(0)->getName().str() == "arrayidx1";
      });
  ASSERT_TRUE(LeakLoadInst);

  // print(arr[0]);
  const auto *LeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 6, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(LeakCallInst);

  const auto LeakLoadID = getMetaDataID(LeakLoadInst);
  const auto LeakCallID = getMetaDataID(LeakCallInst);

  Gt[stoi(LeakCallID)] = {LeakLoadID};

  doAnalysis(HA, Gt, std::monostate{});
}

// XTaint05 is similar to 06, but even harder

TEST_F(IDETaintAnalysisTest, XTaint06) {
  // no leaks expected

  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint06_cpp_dbg.ll"}, EntryPoints);

  doAnalysis(HA, Gt, std::monostate{});
}

/// In the new TaintConfig specifying source/sink/sanitizer properties for extra
/// parameters of C-style variadic functions is not (yet?) supported. So, the
/// tests XTaint07 and XTaint08 are disabled.
TEST_F(IDETaintAnalysisTest, DISABLED_XTaint07) {
  map<int, set<string>> Gt;

  Gt[21] = {"20"};

  // TODO: convert from hardcoded values to using the new dynamic approach (see
  // xtaint20 for example).
  HelperAnalyses HA({PathToLLFiles + "xtaint07_cpp_dbg.ll"}, EntryPoints);

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint08) {
  map<int, set<string>> Gt;

  Gt[24] = {"23"};

  // TODO: convert from hardcoded values to using the new dynamic approach (see
  // xtaint20 for example).
  HelperAnalyses HA({PathToLLFiles + "xtaint08_cpp_dbg.ll"}, EntryPoints);

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint09_1) {
  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint09_1_cpp_dbg.ll"}, EntryPoints);

  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  // sink(*mem);
  const auto *LeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 14, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(LeakCallInst);

  llvm::outs() << "(LeakCallInst->getOperand(0): "
               << *(LeakCallInst->getOperand(0)) << "\n";

  // TODO: ask fabian if this is okay. I am not using getInstAtOrNull here and I
  // am not tying the unit test to the .cpp file.
  const auto *LeakLoadInst = LeakCallInst->getOperand(0);
#if false
  const auto *LeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 14, 0, [LeakCallInst](const llvm::Instruction *Inst) {
        if (Inst) {
          llvm::outs() << "Inst op 0: " << Inst->getOperand(0)->getName().str()
                       << "\n";
        }
        return Inst == LeakCallInst->getOperand(0);
      });
#endif
  ASSERT_TRUE(LeakLoadInst);

  const auto LeakLoadID = getMetaDataID(LeakLoadInst);
  const auto LeakCallID = getMetaDataID(LeakCallInst);

  Gt[stoi(LeakCallID)] = {LeakLoadID};

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint09) {
  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint09_cpp_dbg.ll"}, EntryPoints);
  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  // sink(*mem);
  const auto *LeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 16, 0, [](const llvm::Instruction *Inst) {
        // TODO: this one func call prob has a weird ll with multiple call stuff
        // going on. The PredFn needs to catch that (somehow).
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(LeakCallInst);

#if false
  // %0 = load i32, ptr %call4, align 4, !dbg !1076, !psr.id !1078 | ID: 25
  const auto *LeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 16, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
#endif

  llvm::outs() << "*(LeakCallInst->getOperand(0)): "
               << *(LeakCallInst->getOperand(0)) << "\n";

  // %0 = load i32, ptr %call4, align 4, !dbg !1076, !psr.id !1078 | ID: 25
  const auto *LeakLoadInst = LeakCallInst->getOperand(0);
  ASSERT_TRUE(LeakLoadInst);

  ASSERT_TRUE(false);

  const auto LeakLoadID = getMetaDataID(LeakLoadInst);
  const auto LeakCallID = getMetaDataID(LeakCallInst);

  Gt[stoi(LeakCallID)] = {LeakLoadID};

  doAnalysis(HA, Gt, std::monostate{});
}

#if false
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

  // TODO: convert from hardcoded values to using the new dynamic approach (see
  // xtaint20 for example).
  HelperAnalyses HA({PathToLLFiles + "xtaint10_cpp_dbg.ll"}, EntryPoints);

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, DISABLED_XTaint11) {
  map<int, set<string>> Gt;

  // no leaks expected; actually finds "27" at 28

  // TODO: convert from hardcoded values to using the new dynamic approach (see
  // xtaint20 for example).
  HelperAnalyses HA({PathToLLFiles + "xtaint11_cpp_dbg.ll"}, EntryPoints);

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint12) {
  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint12_cpp_dbg.ll"}, EntryPoints);

  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  // %0 = load i32, ptr %call2, align 4, !dbg !81, !psr.id !82 | ID: 32
  const auto *LeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 19, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  ASSERT_TRUE(LeakLoadInst);

  // sink(*getPtr(&ptaint));
  const auto *LeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 19, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(LeakCallInst);

  const auto LeakLoadID = getMetaDataID(LeakLoadInst);
  const auto LeakCallID = getMetaDataID(LeakCallInst);

  Gt[stoi(LeakCallID)] = {LeakLoadID};

  doAnalysis(HA, Gt, std::monostate{});
}


TEST_F(IDETaintAnalysisTest, XTaint13) {
  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint13_cpp_dbg.ll"}, EntryPoints);
  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  //
  const auto *LeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 17, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  ASSERT_TRUE(LeakLoadInst);

  // sink(x);
  const auto *LeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 17, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(LeakCallInst);

  const auto LeakLoadID = getMetaDataID(LeakLoadInst);
  const auto LeakCallID = getMetaDataID(LeakCallInst);

  Gt[stoi(LeakCallID)] = {LeakLoadID};

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint14) {
  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint14_cpp_dbg.ll"}, EntryPoints);
  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  // %3 = load i32, ptr %x, align 4, !dbg !93, !psr.id !94 | ID: 37
  const auto *LeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 24, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  ASSERT_TRUE(LeakLoadInst);

  // sink(x);
  const auto *LeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 24, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(LeakCallInst);

  const auto LeakLoadID = getMetaDataID(LeakLoadInst);
  const auto LeakCallID = getMetaDataID(LeakCallInst);

  Gt[stoi(LeakCallID)] = {LeakLoadID};

  doAnalysis(HA, Gt, std::monostate{});
}

/// The TaintConfig fails to get all call-sites of Source::get, because it has
/// no CallGraph information
TEST_F(IDETaintAnalysisTest, DISABLED_XTaint15) {
  map<int, set<string>> Gt;

  // TODO: convert from hardcoded values to using the new dynamic approach (see
  // xtaint20 for example).
  Gt[47] = {"46"};

  HelperAnalyses HA({PathToLLFiles + "xtaint15_cpp_dbg.ll"}, EntryPoints);

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint16) {
  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint16_cpp_dbg.ll"}, EntryPoints);
  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  // %1 = load i32, ptr %x, align 4, !dbg !64, !psr.id !65 | ID: 27
  const auto *LeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 13, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  ASSERT_TRUE(LeakLoadInst);

  // we can skip the sanitizer => leak here
  // sink(x);
  const auto *LeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 13, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(LeakCallInst);

  const auto LeakLoadID = getMetaDataID(LeakLoadInst);
  const auto LeakCallID = getMetaDataID(LeakCallInst);

  Gt[stoi(LeakCallID)] = {LeakLoadID};

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint17) {
  map<int, set<string>> Gt;
  HelperAnalyses HA({PathToLLFiles + "xtaint17_cpp_dbg.ll"}, EntryPoints);
  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  // %1 = load i32, ptr %x, align 4, !dbg !76, !psr.id !77 | ID: 31
  const auto *LeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 17, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  ASSERT_TRUE(LeakLoadInst);

  // we can skip the sanitizer => leak here
  // sink(x);
  const auto *LeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 17, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(LeakCallInst);

  const auto LeakLoadID = getMetaDataID(LeakLoadInst);
  const auto LeakCallID = getMetaDataID(LeakCallInst);

  Gt[stoi(LeakCallID)] = {LeakLoadID};

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint18) {
  map<int, set<string>> Gt;

  // TODO: convert from hardcoded values to using the new dynamic approach (see
  // xtaint20 for example).
  // Gt[26] = {"25"};

  HelperAnalyses HA({PathToLLFiles + "xtaint18_cpp_dbg.ll"}, EntryPoints);

  doAnalysis(HA, Gt, std::monostate{});
}

PHASAR_SKIP_TEST(TEST_F(IDETaintAnalysisTest, XTaint19) {
  // Is now the same as XTaint17
  GTEST_SKIP();
  map<int, set<string>> Gt;

  // TODO: convert from hardcoded values to using the new dynamic approach (see
  // xtaint20 for example).
  Gt[22] = {"21"};

  HelperAnalyses HA({PathToLLFiles + "xtaint19_cpp_dbg.ll"}, EntryPoints);

  doAnalysis(HA, Gt, std::monostate{});
})

TEST_F(IDETaintAnalysisTest, XTaint20) {
  map<int, set<string>> Gt;

  HelperAnalyses HA({PathToLLFiles + "xtaint20_cpp_dbg.ll"}, EntryPoints);
  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  // %x = alloca i32, align 4, !psr.id !48 | ID: 16
  const auto *FirstLeakAllocaInst = unittest::getInstAtOrNull(MainFunc, 6, 0);
  ASSERT_TRUE(FirstLeakAllocaInst);

  // srcsink(x); // leak
  const auto *FirstLeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 12, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(FirstLeakCallInst);

  // %0 = load i32, ptr %y, align 4, !dbg !72, !psr.id !73 | ID: 29
  const auto *SecondLeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 13, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  ASSERT_TRUE(SecondLeakLoadInst);

  // sink(y);    // leak
  const auto *SecondLeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 13, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(SecondLeakCallInst);

  const auto FirstLeakAllocaID = getMetaDataID(FirstLeakAllocaInst);
  const auto FirstLeakCallID = getMetaDataID(FirstLeakCallInst);
  const auto SecondLeakLoadID = getMetaDataID(SecondLeakLoadInst);
  const auto SecondLeakCallID = getMetaDataID(SecondLeakCallInst);

  Gt[stoi(FirstLeakCallID)] = {FirstLeakAllocaID};
  Gt[stoi(SecondLeakCallID)] = {SecondLeakLoadID};

  doAnalysis(HA, Gt, std::monostate{});
}

TEST_F(IDETaintAnalysisTest, XTaint21) {
  map<int, set<string>> Gt;

  HelperAnalyses HA({PathToLLFiles + "xtaint21_cpp_dbg.ll"}, EntryPoints);
  const auto &IRDB = HA.getProjectIRDB();
  const auto *MainFunc = IRDB.getFunction("main");

  // %x = alloca i32, align 4, !psr.id !21 | ID: 2
  const auto *FirstLeakAllocaInst = unittest::getInstAtOrNull(MainFunc, 11);
  ASSERT_TRUE(FirstLeakAllocaInst);

  // srcsink(x); // leak
  const auto *FirstLeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 17, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(FirstLeakCallInst);

  // %0 = load i32, ptr %y, align 4, !dbg !45, !psr.id !46 | ID: 15
  const auto *SecondLeakLoadInst = unittest::getInstAtOrNull(
      MainFunc, 18, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::LoadInst>(Inst);
      });
  ASSERT_TRUE(SecondLeakLoadInst);

  // sink(y);    // leak
  const auto *SecondLeakCallInst = unittest::getInstAtOrNull(
      MainFunc, 18, 0, [](const llvm::Instruction *Inst) {
        return llvm::isa<llvm::CallInst>(Inst);
      });
  ASSERT_TRUE(SecondLeakCallInst);

  const auto FirstLeakAllocaID = getMetaDataID(FirstLeakAllocaInst);
  const auto FirstLeakCallID = getMetaDataID(FirstLeakCallInst);
  const auto SecondLeakLoadID = getMetaDataID(SecondLeakLoadInst);
  const auto SecondLeakCallID = getMetaDataID(SecondLeakCallInst);

  Gt[stoi(FirstLeakCallID)] = {FirstLeakAllocaID};
  Gt[stoi(SecondLeakCallID)] = {SecondLeakLoadID};

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

  doAnalysis(HA, Gt, CallBackPairTy{std::move(SourceCB), std::move(SinkCB)});
}

#endif

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
