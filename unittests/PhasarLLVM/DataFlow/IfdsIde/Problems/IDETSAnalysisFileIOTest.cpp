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
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/Instruction.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "SrcCodeLocationEntry.h"
#include "TestConfig.h"
#include "gtest/gtest.h"

#include <memory>
#include <optional>

using namespace std;
using namespace psr;

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

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void TearDown() override {}

  std::map<const llvm::Instruction *, std::map<const llvm::Instruction *, int>>
  srcCodeLocsToInsts(
      const std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
          &GroundTruth) {
    std::map<const llvm::Instruction *,
             std::map<const llvm::Instruction *, int>>
        Converted;

    for (const auto &OuterEntry : GroundTruth) {
      const auto *FirstInst = getInstFromEntryOrNull(std::get<0>(OuterEntry));
      if (FirstInst) {
        for (const auto &InnerEntry : std::get<1>(OuterEntry)) {
          const auto *SecondInst =
              getInstFromEntryOrNull(std::get<0>(InnerEntry));

          if (SecondInst) {
            std::map<const llvm::Instruction *, int> InnerMap = {
                {SecondInst, std::get<1>(InnerEntry)}};

            Converted.insert(
                std::pair<const llvm::Instruction *,
                          std::map<const llvm::Instruction *, int>>(FirstInst,
                                                                    InnerMap));
            continue;
          }

          llvm::outs() << "Line: " << std::get<0>(InnerEntry).Line
                       << "\nColumn: " << std::get<0>(InnerEntry).Column
                       << "\n";
          llvm::report_fatal_error(
              "Second SrcCodeLocationEntry couldn't be converted to an "
              "Instruction.\n");
          // llvm::errs()
          //     << "Second SrcCodeLocationEntry couldn't be converted to an "
          //        "Instruction.\n";
        }
        continue;
      }

      llvm::outs() << "Line: " << std::get<0>(OuterEntry).Line
                   << "\nColumn: " << std::get<0>(OuterEntry).Column << "\n";
      llvm::report_fatal_error(
          "First SrcCodeLocationEntry couldn't be converted to an "
          "Instruction.\n");
      // llvm::errs() << "First SrcCodeLocationEntry couldn't be converted to an
      // "
      //                 "Instruction.\n";
    }

    return Converted;
  }

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  void compareResults(
      const std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
          &GroundTruth,
      IDESolver_P<IDETypeStateAnalysis<CSTDFILEIOTypeStateDescription>>
          &Solver) {
    auto GroundTruthEntries = srcCodeLocsToInsts(GroundTruth);

    int Counter = 0;
    for (const auto &Entry : GroundTruthEntries) {
      std::map<const llvm::Instruction *, int> Results;
      auto GT = std::get<1>(Entry);
      llvm::outs() << "Counter: " << Counter++ << "\n";
      const auto *CurrInst = std::get<0>(Entry);
      for (auto Result : Solver.resultsAt(CurrInst, true)) {
        const auto &FirstResult = std::get<0>(Result);
        const auto &SecondResult = std::get<1>(Result);

        llvm::outs() << "FirstResult:  " << llvmIRToString(FirstResult) << "\n";
        llvm::outs() << "SecondResult: " << SecondResult << "\n";
        if (const auto *CastInst =
                llvm::dyn_cast_or_null<llvm::Instruction>(FirstResult)) {
          if (GT.find(CastInst) != GT.end()) {
            Results.insert(std::pair<const llvm::Instruction *, int>(
                CastInst, int(SecondResult)));
          }
        } else {
          llvm::errs()
              << "[Error]: Couldn't cast FirstResult to Instruction.\n";
        }
      }

      EXPECT_EQ(Results, GT) << "At " << llvmIRToShortString(CurrInst);
    }
  }
}; // Test Fixture

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_01) {
  initialize({PathToLlFiles + "typestate_01_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto File =
      SrcCodeLocationEntry(4, 9, HA->getICFG().getFunction("main"));
  const auto Entry =
      SrcCodeLocationEntry(5, 7, HA->getICFG().getFunction("main"));
  const auto EntryTwo =
      SrcCodeLocationEntry(6, 3, HA->getICFG().getFunction("main"));
  const auto EntryThree =
      SrcCodeLocationEntry(7, 3, HA->getICFG().getFunction("main"));
  GroundTruth.insert({Entry, {{File, IOSTATE::UNINIT}}});
  GroundTruth.insert({EntryTwo, {{File, IOSTATE::OPENED}}});
  GroundTruth.insert({EntryThree, {{File, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_02) {
  initialize({PathToLlFiles + "typestate_02_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto File =
      SrcCodeLocationEntry(4, 9, HA->getICFG().getFunction("main"));
  const auto Entry =
      SrcCodeLocationEntry(6, 3, HA->getICFG().getFunction("main"));
  GroundTruth.insert({Entry, {{File, IOSTATE::OPENED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_03) {
  initialize({PathToLlFiles + "typestate_03_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  // %f = alloca ptr, align 8
  const auto MainFile =
      SrcCodeLocationEntry(6, 9, HA->getICFG().getFunction("main"));
  // %f.addr = alloca ptr, align 8
  const auto FooFile =
      SrcCodeLocationEntry(3, 16, HA->getICFG().getFunction("foo"));
  const auto FooFClose =
      SrcCodeLocationEntry(3, 21, HA->getICFG().getFunction("foo"));
  // %0 = load ptr, ptr %f
  const auto PassFToFClose =
      SrcCodeLocationEntry(3, 28, HA->getICFG().getFunction("foo"));
  // ret void
  const auto FooRet =
      SrcCodeLocationEntry(3, 32, HA->getICFG().getFunction("foo"));
  // %0 = load ptr, ptr %f, align 8
  const auto PassFToFoo =
      SrcCodeLocationEntry(9, 7, HA->getICFG().getFunction("main"));
  // ret i32 0
  const auto Return =
      SrcCodeLocationEntry(11, 3, HA->getICFG().getFunction("main"));
  // Entry in foo()
  GroundTruth.insert({FooFClose, {{FooFile, IOSTATE::OPENED}}});
  // Exit in foo()
  GroundTruth.insert({FooRet,
                      {{FooFile, IOSTATE::CLOSED},
                       {FooFClose, IOSTATE::CLOSED},
                       {PassFToFClose, IOSTATE::CLOSED}}});
  // Exit in main()
  GroundTruth.insert({Return,
                      {{FooFClose, IOSTATE::CLOSED},
                       {MainFile, IOSTATE::CLOSED},
                       {PassFToFoo, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
// TODO: go over old and new ground truth with fabian
#if false
  // llvmtssolver.printReport();
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // Entry in foo()
      {2, {{"foo.0", IOSTATE::OPENED}}},
      // Exit in foo()
      {6,
       {
           {"foo.0", IOSTATE::CLOSED},
           {"2", IOSTATE::CLOSED},
           {"4", IOSTATE::CLOSED},
           //{"8", IOSTATE::CLOSED} // 6 is before 8; so no info avaliable
           // before ret FF
       }},
      // Exit in main()
      {14,
       {{"2", IOSTATE::CLOSED},
        {"8", IOSTATE::CLOSED},
        {"12", IOSTATE::CLOSED}}}};
  compareResults(Gt, Llvmtssolver);
#endif
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_04) {
  initialize({PathToLlFiles + "typestate_04_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto FooArg =
      SrcCodeLocationEntry(4, 16, HA->getICFG().getFunction("foo"));
  const auto FooRet =
      SrcCodeLocationEntry(4, 49, HA->getICFG().getFunction("foo"));
  const auto File =
      SrcCodeLocationEntry(7, 9, HA->getICFG().getFunction("main"));
  const auto FClose =
      SrcCodeLocationEntry(9, 3, HA->getICFG().getFunction("main"));
  const auto Return =
      SrcCodeLocationEntry(10, 3, HA->getICFG().getFunction("main"));
  GroundTruth.insert({FooRet, {{FooArg, IOSTATE::OPENED}}});
  GroundTruth.insert({FClose, {{File, IOSTATE::UNINIT}}});
  GroundTruth.insert({Return, {{File, IOSTATE::ERROR}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_05) {
  initialize({PathToLlFiles + "typestate_05_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto File =
      SrcCodeLocationEntry(6, 9, HA->getICFG().getFunction("main"));
  const auto CallFOpen =
      SrcCodeLocationEntry(7, 7, HA->getICFG().getFunction("main"));
  const auto AfterFOpen =
      SrcCodeLocationEntry(8, 7, HA->getICFG().getFunction("main"));
  const auto LoadFile =
      SrcCodeLocationEntry(9, 12, HA->getICFG().getFunction("main"));
  const auto AfterFClose =
      SrcCodeLocationEntry(10, 3, HA->getICFG().getFunction("main"));
  const auto Return =
      SrcCodeLocationEntry(11, 3, HA->getICFG().getFunction("main"));
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

// TODO: fix
#if false

TEST_F(IDETSAnalysisFileIOTest, DISABLED_HandleTypeState_06) {
  // This test fails due to imprecise points-to information
  initialize({PathToLlFiles + "typestate_06_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto File =
      SrcCodeLocationEntry(6, 9, HA->getICFG().getFunction("main"));
  GroundTruth.insert({Opened, {{File, IOSTATE::UNINIT}}});
  compareResults(GroundTruth, Llvmtssolver);
  const std::map<std::size_t, std::map<std::string, int>> Gt = {
      // Before first fopen()
      {8, {{"5", IOSTATE::UNINIT}, {"6", IOSTATE::UNINIT}}},
      // Before storing the result of the first fopen()
      {9,
       {{"5", IOSTATE::UNINIT},
        {"6", IOSTATE::UNINIT},
        // Return value of first fopen()
        {"8", IOSTATE::OPENED}}},
      // Before second fopen()
      {10,
       {{"5", IOSTATE::OPENED},
        {"6", IOSTATE::UNINIT},
        {"8", IOSTATE::OPENED}}},
      // Before storing the result of the second fopen()
      {11,
       {{"5", IOSTATE::OPENED},
        {"6", IOSTATE::UNINIT},
        // Return value of second fopen()
        {"10", IOSTATE::OPENED}}},
      // Before fclose()
      {13,
       {{"5", IOSTATE::OPENED},
        {"6", IOSTATE::OPENED},
        {"12", IOSTATE::OPENED}}},
      // After if statement
      {14, {{"5", IOSTATE::CLOSED}, {"6", IOSTATE::OPENED}}}};
  compareResults(Gt, Llvmtssolver);
}

#endif

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_07) {
  initialize({PathToLlFiles + "typestate_07_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  // %f.addr = alloca ptr, align 8
  const auto FooFile =
      SrcCodeLocationEntry(3, 16, HA->getICFG().getFunction("foo"));
  // ret void
  const auto FooRet =
      SrcCodeLocationEntry(3, 32, HA->getICFG().getFunction("foo"));
  //  %f = alloca ptr, align 8
  const auto MainFile =
      SrcCodeLocationEntry(6, 9, HA->getICFG().getFunction("main"));
  // %0 = load ptr, ptr %f, align 8
  const auto MainFileLoad =
      SrcCodeLocationEntry(7, 10, HA->getICFG().getFunction("main"));
  // %call = call i32 @fclose(ptr noundef %0)
  const auto CallFClose =
      SrcCodeLocationEntry(7, 3, HA->getICFG().getFunction("main"));
  // %call1 = call noalias ptr @fopen(ptr noundef @.str, ptr noundef @.str.1)
  const auto Call1FOpen =
      SrcCodeLocationEntry(8, 7, HA->getICFG().getFunction("main"));
  // store ptr %call1, ptr %f, align 8
  const auto StoreOfCall1 =
      SrcCodeLocationEntry(8, 5, HA->getICFG().getFunction("main"));
  // %1 = load ptr, ptr %f, align 8
  const auto LoadMainFile =
      SrcCodeLocationEntry(10, 7, HA->getICFG().getFunction("main"));
  // ret i32 0
  const auto MainReturn =
      SrcCodeLocationEntry(12, 3, HA->getICFG().getFunction("main"));

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

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto FooFile =
      SrcCodeLocationEntry(5, 9, HA->getICFG().getFunction("foo"));
  const auto FooRet =
      SrcCodeLocationEntry(7, 3, HA->getICFG().getFunction("foo"));
  const auto MainFile =
      SrcCodeLocationEntry(11, 9, HA->getICFG().getFunction("main"));
  const auto MainReturn =
      SrcCodeLocationEntry(13, 3, HA->getICFG().getFunction("main"));
  GroundTruth.insert({FooRet, {{FooFile, IOSTATE::OPENED}}});
  GroundTruth.insert(
      {MainReturn, {{FooFile, IOSTATE::OPENED}, {MainFile, IOSTATE::UNINIT}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_09) {
  initialize({PathToLlFiles + "typestate_09_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto FooFile =
      SrcCodeLocationEntry(5, 9, HA->getICFG().getFunction("foo"));
  const auto FooRet =
      SrcCodeLocationEntry(7, 3, HA->getICFG().getFunction("foo"));
  const auto MainFile =
      SrcCodeLocationEntry(11, 9, HA->getICFG().getFunction("main"));
  const auto MainReturn =
      SrcCodeLocationEntry(15, 3, HA->getICFG().getFunction("main"));
  GroundTruth.insert({FooRet, {{FooFile, IOSTATE::OPENED}}});
  GroundTruth.insert(
      {MainReturn, {{FooFile, IOSTATE::CLOSED}, {MainFile, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_10) {
  initialize({PathToLlFiles + "typestate_10_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto BarFile =
      SrcCodeLocationEntry(5, 9, HA->getICFG().getFunction("bar"));
  const auto BarRet =
      SrcCodeLocationEntry(6, 3, HA->getICFG().getFunction("bar"));
  const auto FooFile =
      SrcCodeLocationEntry(10, 9, HA->getICFG().getFunction("foo"));
  const auto FooRet =
      SrcCodeLocationEntry(12, 3, HA->getICFG().getFunction("foo"));
  const auto MainFile =
      SrcCodeLocationEntry(16, 9, HA->getICFG().getFunction("main"));
  const auto MainReturn =
      SrcCodeLocationEntry(20, 3, HA->getICFG().getFunction("main"));
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

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto BarFile =
      SrcCodeLocationEntry(4, 16, HA->getICFG().getFunction("bar"));
  const auto BarRet =
      SrcCodeLocationEntry(4, 32, HA->getICFG().getFunction("bar"));
  const auto FooFile =
      SrcCodeLocationEntry(6, 16, HA->getICFG().getFunction("foo"));
  const auto FooRet =
      SrcCodeLocationEntry(6, 49, HA->getICFG().getFunction("foo"));
  const auto MainFile =
      SrcCodeLocationEntry(9, 9, HA->getICFG().getFunction("main"));
  const auto MainReturn =
      SrcCodeLocationEntry(13, 3, HA->getICFG().getFunction("main"));
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

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto BarFile =
      SrcCodeLocationEntry(5, 9, HA->getICFG().getFunction("bar"));
  const auto BarRet =
      SrcCodeLocationEntry(7, 3, HA->getICFG().getFunction("bar"));
  const auto AfterFoo =
      SrcCodeLocationEntry(15, 3, HA->getICFG().getFunction("main"));
  const auto MainFile =
      SrcCodeLocationEntry(13, 9, HA->getICFG().getFunction("main"));
  const auto MainReturn =
      SrcCodeLocationEntry(17, 3, HA->getICFG().getFunction("main"));
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

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto File =
      SrcCodeLocationEntry(4, 9, HA->getICFG().getFunction("main"));
  const auto BeforeFirstFClose =
      SrcCodeLocationEntry(7, 3, HA->getICFG().getFunction("main"));
  const auto BeforeSecondFClose =
      SrcCodeLocationEntry(8, 3, HA->getICFG().getFunction("main"));
  const auto Return =
      SrcCodeLocationEntry(10, 3, HA->getICFG().getFunction("main"));
  GroundTruth.insert({BeforeFirstFClose, {{File, IOSTATE::OPENED}}});
  GroundTruth.insert({BeforeSecondFClose, {{File, IOSTATE::CLOSED}}});
  GroundTruth.insert({Return, {{File, IOSTATE::ERROR}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_14) {
  initialize({PathToLlFiles + "typestate_14_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto File =
      SrcCodeLocationEntry(4, 9, HA->getICFG().getFunction("main"));
  const auto BeforeFirstFOpen =
      SrcCodeLocationEntry(5, 5, HA->getICFG().getFunction("main"));
  const auto BeforeSecondFOpen =
      SrcCodeLocationEntry(6, 5, HA->getICFG().getFunction("main"));
  const auto BeforeFClose =
      SrcCodeLocationEntry(7, 3, HA->getICFG().getFunction("main"));
  const auto Return =
      SrcCodeLocationEntry(9, 3, HA->getICFG().getFunction("main"));
  GroundTruth.insert({BeforeFirstFOpen, {{File, IOSTATE::UNINIT}}});
  GroundTruth.insert({BeforeSecondFOpen, {{File, IOSTATE::OPENED}}});
  GroundTruth.insert({BeforeFClose,
                      {{File, IOSTATE::OPENED},
                       {BeforeFirstFOpen, IOSTATE::OPENED},
                       {BeforeSecondFOpen, IOSTATE::OPENED}}});
  GroundTruth.insert({Return,
                      {{File, IOSTATE::CLOSED},
                       {BeforeFirstFOpen, IOSTATE::CLOSED},
                       {BeforeSecondFOpen, IOSTATE::CLOSED}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_15) {
  initialize({PathToLlFiles + "typestate_15_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  // 5: %f = alloca ptr, align 8
  const auto File =
      SrcCodeLocationEntry(4, 9, HA->getICFG().getFunction("main"));
  // %call = call noalias ptr @fopen
  const auto FOpen =
      SrcCodeLocationEntry(5, 7, HA->getICFG().getFunction("main"));
  // %0 = load ptr, ptr %f, align 8
  const auto LoadFile =
      SrcCodeLocationEntry(6, 10, HA->getICFG().getFunction("main"));
  // %call2 = call noalias ptr @fopen
  const auto SecondFOpen =
      SrcCodeLocationEntry(7, 7, HA->getICFG().getFunction("main"));
  // store ptr %call2, ptr %f, align 8
  const auto StoreSecondFOpen =
      SrcCodeLocationEntry(7, 5, HA->getICFG().getFunction("main"));
  // %1 = load ptr, ptr %f, align 8
  const auto SecondLoadFile =
      SrcCodeLocationEntry(8, 10, HA->getICFG().getFunction("main"));
  // ret i32 0
  const auto Return =
      SrcCodeLocationEntry(10, 3, HA->getICFG().getFunction("main"));

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

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto FooFile =
      SrcCodeLocationEntry(4, 16, HA->getICFG().getFunction("foo"));
  const auto FooExit =
      SrcCodeLocationEntry(11, 1, HA->getICFG().getFunction("foo"));
  const auto MainFile =
      SrcCodeLocationEntry(14, 9, HA->getICFG().getFunction("main"));
  const auto MainReturn =
      SrcCodeLocationEntry(19, 3, HA->getICFG().getFunction("main"));
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

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto FooFile =
      SrcCodeLocationEntry(4, 16, HA->getICFG().getFunction("foo"));
  const auto File =
      SrcCodeLocationEntry(8, 9, HA->getICFG().getFunction("main"));
  const auto FOpenFile =
      SrcCodeLocationEntry(8, 9, HA->getICFG().getFunction("main"));
  const auto BeforeLoop =
      SrcCodeLocationEntry(14, 3, HA->getICFG().getFunction("main"));
  const auto BeforeFGetC =
      SrcCodeLocationEntry(14, 13, HA->getICFG().getFunction("main"));
  const auto MainReturn =
      SrcCodeLocationEntry(17, 3, HA->getICFG().getFunction("main"));
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

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto FooReturn =
      SrcCodeLocationEntry(11, 1, HA->getICFG().getFunction("foo"));
  const auto FooFile =
      SrcCodeLocationEntry(4, 16, HA->getICFG().getFunction("foo"));
  const auto MainFile =
      SrcCodeLocationEntry(14, 9, HA->getICFG().getFunction("main"));
  const auto MainReturn =
      SrcCodeLocationEntry(19, 3, HA->getICFG().getFunction("main"));
  GroundTruth.insert({FooReturn, {{FooFile, IOSTATE::BOT}}});
  GroundTruth.insert(
      {MainReturn, {{MainFile, IOSTATE::BOT}, {FooFile, IOSTATE::BOT}}});
  compareResults(GroundTruth, Llvmtssolver);
}

TEST_F(IDETSAnalysisFileIOTest, HandleTypeState_19) {
  initialize({PathToLlFiles + "typestate_19_c_dbg.ll"});
  IDESolver Llvmtssolver(*TSProblem, &HA->getICFG());
  Llvmtssolver.solve();

  std::map<SrcCodeLocationEntry, std::map<SrcCodeLocationEntry, int>>
      GroundTruth;
  const auto FooFile =
      SrcCodeLocationEntry(4, 16, HA->getICFG().getFunction("foo"));
  const auto MainFile =
      SrcCodeLocationEntry(7, 9, HA->getICFG().getFunction("main"));
  const auto WhileCond =
      SrcCodeLocationEntry(11, 3, HA->getICFG().getFunction("main"));
  const auto StoreCall =
      SrcCodeLocationEntry(11, 13, HA->getICFG().getFunction("main"));
  const auto MainReturn =
      SrcCodeLocationEntry(18, 3, HA->getICFG().getFunction("main"));

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
