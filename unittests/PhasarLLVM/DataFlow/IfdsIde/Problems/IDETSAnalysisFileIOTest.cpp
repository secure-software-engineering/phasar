/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/DebugOutput.h"

#include "llvm/IR/Instruction.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <optional>

using namespace psr;
using namespace psr::unittest;

/* ============== TEST FIXTURE ============== */
class IDETSAnalysisFileIOTest : public ::testing::Test {
protected:
  static constexpr auto PathToLlFiles =
      PHASAR_BUILD_SUBFOLDER("typestate_analysis_fileio/");
  const std::vector<std::string> EntryPoints = {"main"};

  std::optional<HelperAnalyses> HA;
  CSTDFILEIOTypeStateDescription CSTDFILEIODesc{};
  std::optional<IDETypeStateAnalysis<CSTDFILEIOTypeStateDescription>> TSProblem;
  enum IOSTATE {
    TOP = 42,
    UNINIT = 0,
    OPENED = 1,
    CLOSED = 2,
    ERROR = 3,
    BOT = 4
  };

  IDETSAnalysisFileIOTest() = default;
  ~IDETSAnalysisFileIOTest() override = default;

  void initialize(const llvm::Twine &IRFile) {
    HA.emplace(IRFile, EntryPoints);

    TSProblem = createAnalysisProblem<
        IDETypeStateAnalysis<CSTDFILEIOTypeStateDescription>>(
        *HA, &CSTDFILEIODesc, EntryPoints);
  }

  using GroundTruthMapTy =
      std::map<TestingSrcLocation, std::map<TestingSrcLocation, int>>;

  [[nodiscard]] static inline auto convertTestingLocationMapMapInIR(
      const GroundTruthMapTy &Locs,
      const ProjectIRDBBase<LLVMProjectIRDB> &IRDB) {
    std::map<const llvm::Instruction *, std::map<const llvm::Value *, int>> Ret;
    llvm::transform(
        Locs, std::inserter(Ret, Ret.end()), [&](const auto &LocAndSet) {
          const auto &[InstLoc, InnerMap] = LocAndSet;
          const auto *LocVal = llvm::dyn_cast_if_present<llvm::Instruction>(
              testingLocInIR(InstLoc, IRDB));
          std::map<const llvm::Value *, int> ConvMap;
          for (const auto &[FactLoc, Val] : InnerMap) {
            if (const auto *Fact = testingLocInIR(FactLoc, IRDB)) {
              ConvMap.try_emplace(Fact, Val);
            }
          }
          return std::make_pair(LocVal, std::move(ConvMap));
        });
    return Ret;
  }

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  void compareResults(
      const GroundTruthMapTy &GroundTruth,
      IDESolver_P<IDETypeStateAnalysis<CSTDFILEIOTypeStateDescription>>
          &Solver) {
    auto GroundTruthEntries =
        convertTestingLocationMapMapInIR(GroundTruth, HA->getProjectIRDB());

    for (const auto &[CurrInst, GT] : GroundTruthEntries) {
      std::map<const llvm::Value *, int> Results;

      for (const auto &[ResFact, ResState] : Solver.resultsAt(CurrInst, true)) {
        if (GT.count(ResFact)) {
          Results.try_emplace(ResFact, int(ResState));
        }
      }

      EXPECT_EQ(Results, GT)
          << "At " << llvmIRToShortString(CurrInst) << ": Expected "
          << PrettyPrinter{GT} << "; got: " << PrettyPrinter{Results};
    }
  }
}; // Test Fixture

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_01) {
  initialize({PathToLlFiles + "typestate_01_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto File = LineColFun{4, 9, "main"};
  const auto Entry = LineColFun{5, 7, "main"};
  const auto EntryTwo = LineColFun{6, 3, "main"};
  const auto EntryThree = LineColFun{7, 3, "main"};
  GroundTruth.insert({Entry, {{File, IOSTATE::UNINIT}}});
  GroundTruth.insert({EntryTwo, {{File, IOSTATE::OPENED}}});
  GroundTruth.insert({EntryThree, {{File, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_02) {
  initialize({PathToLlFiles + "typestate_02_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto File = LineColFun{4, 9, "main"};
  const auto Entry = LineColFun{6, 3, "main"};
  GroundTruth.insert({Entry, {{File, IOSTATE::OPENED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_03) {
  initialize({PathToLlFiles + "typestate_03_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  // %f = alloca ptr, align 8
  const auto MainFile = LineColFun{6, 9, "main"};
  // %f.addr = alloca ptr, align 8
  //   const auto FooFile = LineColFun{3, 16, "foo"};
  // const auto FooFClose =
  //     LineColFun{3, 21, "foo"};
  // %0 = load ptr, ptr %f
  const auto PassFToFClose = LineColFun{3, 28, "foo"};
  // ret void
  const auto FooRet = LineColFun{3, 32, "foo"};
  // %0 = load ptr, ptr %f, align 8
  const auto PassFToFoo = LineColFun{9, 7, "main"};
  // ret i32 0
  const auto Return = LineColFun{11, 3, "main"};
  // Entry in foo()
  // GroundTruth.insert({FooFClose, {{FooFile, IOSTATE::OPENED}}});
  // Exit in foo()
  GroundTruth.insert({FooRet,
                      {// {FooFile, IOSTATE::CLOSED},
                       {PassFToFClose, IOSTATE::CLOSED}}});
  // Exit in main()
  GroundTruth.insert({Return,
                      {// {FooFClose, IOSTATE::CLOSED},
                       {MainFile, IOSTATE::CLOSED},
                       {PassFToFoo, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_04) {
  initialize({PathToLlFiles + "typestate_04_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto FooArg = LineColFun{4, 16, "foo"};
  const auto FooRet = LineColFun{4, 49, "foo"};
  const auto File = LineColFun{7, 9, "main"};
  const auto FClose = LineColFun{9, 3, "main"};
  const auto Return = LineColFun{10, 3, "main"};
  GroundTruth.insert({FooRet, {{FooArg, IOSTATE::OPENED}}});
  GroundTruth.insert({FClose, {{File, IOSTATE::UNINIT}}});
  GroundTruth.insert({Return, {{File, IOSTATE::ERROR}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_05) {
  initialize({PathToLlFiles + "typestate_05_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto File = LineColFun{6, 9, "main"};
  const auto CallFOpen = LineColFun{7, 7, "main"};
  const auto AfterFOpen = LineColFun{8, 7, "main"};
  const auto LoadFile = LineColFun{9, 12, "main"};
  const auto AfterFClose = LineColFun{10, 3, "main"};
  const auto Return = LineColFun{11, 3, "main"};
  GroundTruth.insert(
      {AfterFOpen, {{File, IOSTATE::OPENED}, {CallFOpen, IOSTATE::OPENED}}});
  GroundTruth.insert({AfterFClose,
                      {{File, IOSTATE::CLOSED},
                       {CallFOpen, IOSTATE::CLOSED},
                       {LoadFile, IOSTATE::CLOSED}}});
  GroundTruth.insert(
      {Return, {{File, IOSTATE::BOT}, {CallFOpen, IOSTATE::BOT}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, DISABLED_HandleTypeState_06) {
  // This test fails due to imprecise points-to information
  initialize({PathToLlFiles + "typestate_06_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;

  // %f = alloca ptr, align 8
  const auto FileF = LineColFun{5, 9, "main"};
  // %d = alloca ptr, align 8
  const auto FileD = LineColFun{6, 9, "main"};
  // %call = call noalias ptr @fopen(ptr noundef @.str, ptr noundef @.str.1)
  const auto FirstFOpenCall = LineColFun{7, 7, "main"};
  // store ptr %call, ptr %f, align 8
  const auto StoreFirstFOpenRetVal = LineColFun{7, 5, "main"};
  // %call1 = call noalias ptr @fopen(ptr noundef @.str.2, ptr noundef @.str.3)
  const auto SecondFOpenCall = LineColFun{8, 7, "main"};
  // store ptr %call1, ptr %d, align 8
  const auto StoreSecondFOpenRetVal = LineColFun{8, 5, "main"};
  // %0 = load ptr, ptr %f, align 8
  const auto LoadFileF = LineColFun{10, 10, "main"};
  // %call2 = call i32 @fclose(ptr noundef %0)
  const auto CallFClose = LineColFun{10, 3, "main"};
  // ret i32 0
  const auto Return = LineColFun{12, 3, "main"};

  GroundTruth.insert({FirstFOpenCall, {{FileF, IOSTATE::UNINIT}}});
  GroundTruth.insert({FirstFOpenCall, {{FileD, IOSTATE::UNINIT}}});

  GroundTruth.insert({StoreFirstFOpenRetVal, {{FileF, IOSTATE::UNINIT}}});
  GroundTruth.insert({StoreFirstFOpenRetVal, {{FileD, IOSTATE::UNINIT}}});
  GroundTruth.insert(
      {StoreFirstFOpenRetVal, {{FirstFOpenCall, IOSTATE::OPENED}}});

  GroundTruth.insert({SecondFOpenCall, {{FileF, IOSTATE::OPENED}}});
  GroundTruth.insert({SecondFOpenCall, {{FileD, IOSTATE::UNINIT}}});
  GroundTruth.insert({SecondFOpenCall, {{FirstFOpenCall, IOSTATE::OPENED}}});

  GroundTruth.insert({StoreSecondFOpenRetVal, {{FileF, IOSTATE::OPENED}}});
  GroundTruth.insert({StoreSecondFOpenRetVal, {{FileD, IOSTATE::UNINIT}}});
  GroundTruth.insert(
      {StoreSecondFOpenRetVal, {{SecondFOpenCall, IOSTATE::OPENED}}});

  GroundTruth.insert({CallFClose, {{FileF, IOSTATE::OPENED}}});
  GroundTruth.insert({CallFClose, {{FileD, IOSTATE::UNINIT}}});
  GroundTruth.insert({CallFClose, {{LoadFileF, IOSTATE::OPENED}}});

  GroundTruth.insert({Return, {{FileF, IOSTATE::OPENED}}});
  GroundTruth.insert({Return, {{FileD, IOSTATE::UNINIT}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_07) {
  initialize({PathToLlFiles + "typestate_07_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  // %f.addr = alloca ptr, align 8
  const auto FooFile = LineColFun{3, 16, "foo"};
  // ret void
  const auto FooRet = LineColFun{3, 32, "foo"};
  //  %f = alloca ptr, align 8
  const auto MainFile = LineColFun{6, 9, "main"};
  // %0 = load ptr, ptr %f, align 8
  const auto MainFileLoad = LineColFun{7, 10, "main"};
  // %call = call i32 @fclose(ptr noundef %0)
  const auto CallFClose = LineColFun{7, 3, "main"};
  // %call1 = call noalias ptr @fopen(ptr noundef @.str, ptr noundef @.str.1)
  const auto Call1FOpen = LineColFun{8, 7, "main"};
  // store ptr %call1, ptr %f, align 8
  const auto StoreOfCall1 = LineColFun{8, 5, "main"};
  // %1 = load ptr, ptr %f, align 8
  const auto LoadMainFile = LineColFun{10, 7, "main"};
  // ret i32 0
  const auto MainReturn = LineColFun{12, 3, "main"};

  GroundTruth.insert({FooRet, {{FooFile, IOSTATE::CLOSED}}});
  GroundTruth.insert(
      {CallFClose,
       {{MainFile, IOSTATE::UNINIT}, {MainFileLoad, IOSTATE::UNINIT}}});
  GroundTruth.insert(
      {Call1FOpen,
       {{MainFile, IOSTATE::ERROR}, {MainFileLoad, IOSTATE::ERROR}}});
  GroundTruth.insert({StoreOfCall1,
                      {{MainFile, IOSTATE::ERROR},
                       {MainFileLoad, IOSTATE::ERROR},
                       {Call1FOpen, IOSTATE::OPENED}}});
  GroundTruth.insert({LoadMainFile,
                      {{MainFile, IOSTATE::OPENED},
                       {MainFileLoad, IOSTATE::ERROR},
                       {Call1FOpen, IOSTATE::OPENED}}});
  GroundTruth.insert(
      {MainReturn, {{MainFile, IOSTATE::CLOSED}, {FooFile, IOSTATE::CLOSED}}});
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_08) {
  initialize({PathToLlFiles + "typestate_08_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto FooFile = LineColFun{5, 9, "foo"};
  const auto FooRet = LineColFun{7, 3, "foo"};
  const auto MainFile = LineColFun{11, 9, "main"};
  const auto MainReturn = LineColFun{13, 3, "main"};
  GroundTruth.insert({FooRet, {{FooFile, IOSTATE::OPENED}}});
  GroundTruth.insert(
      {MainReturn, {{FooFile, IOSTATE::OPENED}, {MainFile, IOSTATE::UNINIT}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_09) {
  initialize({PathToLlFiles + "typestate_09_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto FooFile = LineColFun{5, 9, "foo"};
  const auto FooRet = LineColFun{7, 3, "foo"};
  const auto MainFile = LineColFun{11, 9, "main"};
  const auto MainReturn = LineColFun{15, 3, "main"};
  GroundTruth.insert({FooRet, {{FooFile, IOSTATE::OPENED}}});
  GroundTruth.insert(
      {MainReturn, {{FooFile, IOSTATE::CLOSED}, {MainFile, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_10) {
  initialize({PathToLlFiles + "typestate_10_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto BarFile = LineColFun{5, 9, "bar"};
  const auto BarRet = LineColFun{6, 3, "bar"};
  const auto FooFile = LineColFun{10, 9, "foo"};
  const auto FooRet = LineColFun{12, 3, "foo"};
  const auto MainFile = LineColFun{16, 9, "main"};
  const auto MainReturn = LineColFun{20, 3, "main"};
  GroundTruth.insert({BarRet, {{BarFile, IOSTATE::UNINIT}}});
  GroundTruth.insert({FooRet, {{FooFile, IOSTATE::OPENED}}});
  GroundTruth.insert({MainReturn,
                      {{BarFile, IOSTATE::CLOSED},
                       {FooFile, IOSTATE::CLOSED},
                       {MainFile, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_11) {
  initialize({PathToLlFiles + "typestate_11_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto BarFile = LineColFun{4, 16, "bar"};
  const auto BarRet = LineColFun{4, 32, "bar"};
  const auto FooFile = LineColFun{6, 16, "foo"};
  const auto FooRet = LineColFun{6, 49, "foo"};
  const auto MainFile = LineColFun{9, 9, "main"};
  const auto MainReturn = LineColFun{13, 3, "main"};
  GroundTruth.insert({BarRet, {{BarFile, IOSTATE::ERROR}}});
  GroundTruth.insert({FooRet, {{FooFile, IOSTATE::OPENED}}});
  GroundTruth.insert({MainReturn,
                      {{BarFile, IOSTATE::ERROR},
                       {FooFile, IOSTATE::ERROR},
                       {MainFile, IOSTATE::ERROR}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_12) {
  initialize({PathToLlFiles + "typestate_12_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto BarFile = LineColFun{5, 9, "bar"};
  const auto BarRet = LineColFun{7, 3, "bar"};
  const auto AfterFoo = LineColFun{15, 3, "main"};
  const auto MainFile = LineColFun{13, 9, "main"};
  const auto MainReturn = LineColFun{17, 3, "main"};
  GroundTruth.insert({BarRet, {{BarFile, IOSTATE::OPENED}}});
  GroundTruth.insert(
      {AfterFoo, {{MainFile, IOSTATE::OPENED}, {BarFile, IOSTATE::OPENED}}});
  GroundTruth.insert(
      {MainReturn, {{MainFile, IOSTATE::CLOSED}, {BarFile, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_13) {
  initialize({PathToLlFiles + "typestate_13_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto File = LineColFun{4, 9, "main"};
  const auto BeforeFirstFClose = LineColFun{7, 3, "main"};
  const auto BeforeSecondFClose = LineColFun{8, 3, "main"};
  const auto Return = LineColFun{10, 3, "main"};
  GroundTruth.insert({BeforeFirstFClose, {{File, IOSTATE::OPENED}}});
  GroundTruth.insert({BeforeSecondFClose, {{File, IOSTATE::CLOSED}}});
  GroundTruth.insert({Return, {{File, IOSTATE::ERROR}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_14) {
  initialize({PathToLlFiles + "typestate_14_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto File = LineColFun{4, 9, "main"};
  const auto BeforeFirstFOpen = LineColFun{5, 5, "main"};
  const auto BeforeSecondFOpen = LineColFun{6, 5, "main"};
  const auto BeforeFClose = LineColFun{7, 3, "main"};
  const auto Return = LineColFun{9, 3, "main"};
  GroundTruth.insert({BeforeFirstFOpen, {{File, IOSTATE::UNINIT}}});
  GroundTruth.insert({BeforeSecondFOpen, {{File, IOSTATE::OPENED}}});
  GroundTruth.insert({BeforeFClose,
                      {{File, IOSTATE::OPENED},
                       {LineColFun{5, 7, "main"}, IOSTATE::OPENED},
                       {LineColFun{6, 7, "main"}, IOSTATE::OPENED}}});
  GroundTruth.insert({Return,
                      {{File, IOSTATE::CLOSED},
                       {LineColFun{5, 7, "main"}, IOSTATE::CLOSED},
                       {LineColFun{6, 7, "main"}, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_15) {
  initialize({PathToLlFiles + "typestate_15_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  // 5: %f = alloca ptr, align 8
  const auto File = LineColFun{4, 9, "main"};
  // %call = call noalias ptr @fopen
  const auto FOpen = LineColFun{5, 7, "main"};
  // %0 = load ptr, ptr %f, align 8
  const auto LoadFile = LineColFun{6, 10, "main"};
  // %call2 = call noalias ptr @fopen
  const auto SecondFOpen = LineColFun{7, 7, "main"};
  // store ptr %call2, ptr %f, align 8
  const auto StoreSecondFOpen = LineColFun{7, 5, "main"};
  // %1 = load ptr, ptr %f, align 8
  const auto SecondLoadFile = LineColFun{8, 10, "main"};
  // ret i32 0
  const auto Return = LineColFun{10, 3, "main"};

  GroundTruth.insert(
      {LoadFile, {{File, IOSTATE::OPENED}, {FOpen, IOSTATE::OPENED}}});
  GroundTruth.insert({SecondFOpen,
                      {{File, IOSTATE::CLOSED},
                       {FOpen, IOSTATE::CLOSED},
                       {LoadFile, IOSTATE::CLOSED}}});
  GroundTruth.insert({StoreSecondFOpen,
                      {{File, IOSTATE::CLOSED},
                       {FOpen, IOSTATE::CLOSED},
                       {LoadFile, IOSTATE::CLOSED},
                       {SecondFOpen, IOSTATE::OPENED}}});
  GroundTruth.insert({SecondLoadFile,
                      {{File, IOSTATE::OPENED},
                       {FOpen, IOSTATE::CLOSED},
                       {LoadFile, IOSTATE::CLOSED},
                       {SecondFOpen, IOSTATE::OPENED}}});
  GroundTruth.insert({Return,
                      {{File, IOSTATE::CLOSED},
                       {FOpen, IOSTATE::ERROR},
                       {LoadFile, IOSTATE::ERROR},
                       {SecondFOpen, IOSTATE::CLOSED},
                       {SecondLoadFile, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_16) {

  /// TODO: After the EF fix everything is BOT; --> Make the TSA more precise!

  initialize({PathToLlFiles + "typestate_16_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto FooFile = LineColFun{4, 16, "foo"};
  const auto FooExit = LineColFun{11, 1, "foo"};
  const auto MainFile = LineColFun{14, 9, "main"};
  const auto MainReturn = LineColFun{19, 3, "main"};
  // At exit in foo()
  GroundTruth.insert({FooExit, {{FooFile, IOSTATE::BOT}}});
  // At exit in main()
  GroundTruth.insert(
      {MainReturn, {{FooFile, IOSTATE::BOT}, {MainFile, IOSTATE::BOT}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_17) {
  initialize({PathToLlFiles + "typestate_17_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto FooFile = LineColFun{4, 16, "foo"};
  const auto File = LineColFun{8, 9, "main"};
  const auto FOpenFile = LineColFun{8, 9, "main"};
  const auto BeforeLoop = LineColFun{14, 3, "main"};
  const auto BeforeFGetC = LineColFun{14, 13, "main"};
  const auto MainReturn = LineColFun{17, 3, "main"};
  GroundTruth.insert({BeforeLoop,
                      {{FooFile, IOSTATE::CLOSED},
                       {File, IOSTATE::CLOSED},
                       {FOpenFile, IOSTATE::CLOSED}}});
  GroundTruth.insert({BeforeFGetC,
                      {{FooFile, IOSTATE::BOT},
                       {File, IOSTATE::BOT},
                       {FOpenFile, IOSTATE::BOT}}});
  GroundTruth.insert({MainReturn,
                      {{FooFile, IOSTATE::BOT},
                       {File, IOSTATE::BOT},
                       {FOpenFile, IOSTATE::BOT}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_18) {
  /// TODO: After the EF fix everything is BOT; --> Make the TSA more precise!

  initialize({PathToLlFiles + "typestate_18_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto FooReturn = LineColFun{11, 1, "foo"};
  const auto FooFile = LineColFun{4, 16, "foo"};
  const auto MainFile = LineColFun{14, 9, "main"};
  const auto MainReturn = LineColFun{19, 3, "main"};
  GroundTruth.insert({FooReturn, {{FooFile, IOSTATE::BOT}}});
  GroundTruth.insert(
      {MainReturn, {{MainFile, IOSTATE::BOT}, {FooFile, IOSTATE::BOT}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_19) {
  initialize({PathToLlFiles + "typestate_19_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  GroundTruthMapTy GroundTruth;
  const auto FooFile = LineColFun{4, 16, "foo"};
  const auto MainFile = LineColFun{7, 9, "main"};
  const auto WhileCond = LineColFun{11, 3, "main"};
  const auto StoreCall = LineColFun{11, 13, "main"};
  const auto MainReturn = LineColFun{18, 3, "main"};

  GroundTruth.insert({WhileCond, {{MainFile, IOSTATE::UNINIT}}});
  GroundTruth.insert({StoreCall, {{MainFile, IOSTATE::BOT}}});
  GroundTruth.insert(
      {MainReturn, {{FooFile, IOSTATE::CLOSED}, {MainFile, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
