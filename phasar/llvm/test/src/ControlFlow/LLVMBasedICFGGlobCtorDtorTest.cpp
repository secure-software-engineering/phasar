/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Config/Configuration.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <string>
#include <vector>

using namespace std;
using namespace psr;

class LLVMBasedICFGGlobCtorDtorTest : public ::testing::Test {
protected:
  static constexpr auto PathToLLFiles = PHASAR_BUILD_SUBFOLDER("globals/");

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void ensureFunctionOrdering(
      llvm::Function *F, LLVMBasedICFG &ICFG,
      std::initializer_list<std::pair<llvm::StringRef, llvm::StringRef>>
          FixedOrdering) {

    auto CallSites = ICFG.getCallsFromWithin(F);

    llvm::StringMap<const llvm::CallBase *> CSByCalleeName;

    for (const auto *CS : CallSites) {
      const auto *Call = llvm::cast<llvm::CallBase>(CS);

      if (const auto *Callee = Call->getCalledFunction()) {
        // Assume, we have no duplicates
        CSByCalleeName[Callee->getName()] = Call;
      }
    }

    llvm::DominatorTree Dom(*F);

    for (auto [First, Second] : FixedOrdering) {
      EXPECT_TRUE(CSByCalleeName.count(First));
      EXPECT_TRUE(CSByCalleeName.count(Second));

      if (CSByCalleeName.count(First) && CSByCalleeName.count(Second)) {
        EXPECT_TRUE(
            Dom.dominates(CSByCalleeName[First], CSByCalleeName[Second]));
      }
    }
  }
};

TEST_F(LLVMBasedICFGGlobCtorDtorTest, CtorTest) {

  LLVMProjectIRDB IRDB({PathToLLFiles + "globals_ctor_1.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);

  auto *GlobalCtor = IRDB.getFunction(LLVMBasedICFG::GlobalCRuntimeModelName);
  ASSERT_TRUE(GlobalCtor != nullptr);

  // GlobalCtor->print(llvm::outs());

  ensureFunctionOrdering(GlobalCtor, ICFG,
                         {{"_GLOBAL__sub_I_globals_ctor_1.cpp", "main"},
                          {"main", "__psrCRuntimeGlobalDtorsModel"}});
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, CtorTest2) {

  llvm::LLVMContext Ctx;
  auto M1 = LLVMProjectIRDB::getParsedIRModuleOrNull(
      PathToLLFiles + "globals_ctor_2_1.ll", Ctx);
  auto M2 = LLVMProjectIRDB::getParsedIRModuleOrNull(
      PathToLLFiles + "globals_ctor_2_2.ll", Ctx);

  ASSERT_NE(nullptr, M1);
  ASSERT_NE(nullptr, M2);

  auto LinkerError = llvm::Linker::linkModules(*M1, std::move(M2));
  ASSERT_FALSE(LinkerError);

  LLVMProjectIRDB IRDB(std::move(M1), /*DoPreprocessing*/ true);
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);

  auto *GlobalCtor = IRDB.getFunction(LLVMBasedICFG::GlobalCRuntimeModelName);
  ASSERT_TRUE(GlobalCtor != nullptr);

  // GlobalCtor->print(llvm::outs());

  ensureFunctionOrdering(GlobalCtor, ICFG,
                         {{"_GLOBAL__sub_I_globals_ctor_2_1.cpp", "main"},
                          {"_GLOBAL__sub_I_globals_ctor_2_2.cpp", "main"}});
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, DtorTest1) {

  LLVMProjectIRDB IRDB({PathToLLFiles + "globals_dtor_1.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);

  auto *GlobalCtor = IRDB.getFunction(LLVMBasedICFG::GlobalCRuntimeModelName);
  ASSERT_NE(nullptr, GlobalCtor);

  // GlobalCtor->print(llvm::outs());

  ensureFunctionOrdering(
      GlobalCtor, ICFG,
      {{"_GLOBAL__sub_I_globals_dtor_1.cpp", "main"},
       {"main", "__psrGlobalDtorsCaller.globals_dtor_1.ll"}});

  auto *GlobalDtor =
      IRDB.getFunction("__psrGlobalDtorsCaller.globals_dtor_1.ll");

  ASSERT_NE(nullptr, GlobalDtor);

  auto DtorCallSites = ICFG.getCallsFromWithin(GlobalDtor);
  EXPECT_EQ(2, std::count_if(DtorCallSites.begin(), DtorCallSites.end(),
                             [](const llvm::Instruction *CS) {
                               auto Call = llvm::cast<llvm::CallBase>(CS);
                               return (Call->getCalledFunction() &&
                                       Call->getCalledFunction()->getName() ==
                                           "_ZN3FooD2Ev");
                             }));
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, LCATest1) {

  LLVMProjectIRDB IRDB({PathToLLFiles + "globals_lca_1.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);

  IDELinearConstantAnalysis Problem(
      &IRDB, &ICFG, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem, &ICFG);

  Solver.solve();

  // Solver.dumpResults();

  const auto *FooInit = IRDB.getInstruction(6);
  const auto *LoadX = IRDB.getInstruction(11);
  const auto *End = IRDB.getInstruction(13);
  const auto *Foo = IRDB.getGlobalVariableDefinition("foo");

  auto FooValueAfterInit = Solver.resultAt(FooInit, Foo);

  EXPECT_EQ(42, FooValueAfterInit)
      << "Value of foo at " << llvmIRToString(FooInit) << " is not 42";

  auto XValueAtEnd = Solver.resultAt(End, LoadX);
  auto FooValueAtEnd = Solver.resultAt(End, Foo);

  EXPECT_EQ(42, FooValueAtEnd)
      << "Value of foo at " << llvmIRToString(End) << " is not 42";
  EXPECT_EQ(43, XValueAtEnd)
      << "Value of x at " << llvmIRToString(End) << " is not 43";
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, LCATest2) {

  LLVMProjectIRDB IRDB({PathToLLFiles + "globals_lca_2.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);
  IDELinearConstantAnalysis Problem(
      &IRDB, &ICFG, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem, &ICFG);

  Solver.solve();

  // Solver.dumpResults();

  const auto *FooInit = IRDB.getInstruction(7);
  const auto *BarInit = IRDB.getInstruction(11);
  const auto *LoadX = IRDB.getInstruction(20);
  const auto *LoadY = IRDB.getInstruction(21);
  const auto *End = IRDB.getInstruction(23);
  const auto *Foo = IRDB.getGlobalVariableDefinition("foo");
  const auto *Bar = IRDB.getGlobalVariableDefinition("bar");

  auto FooValueAfterInit = Solver.resultAt(FooInit, Foo);
  auto BarValueAfterInit = Solver.resultAt(BarInit, Bar);

  EXPECT_EQ(42, FooValueAfterInit);
  EXPECT_EQ(45, BarValueAfterInit);

  auto XValueAtEnd = Solver.resultAt(End, LoadX);
  auto FooValueAtEnd = Solver.resultAt(End, Foo);
  auto YValueAtEnd = Solver.resultAt(End, LoadY);
  auto BarValueAtEnd = Solver.resultAt(End, Bar);

  EXPECT_EQ(42, FooValueAtEnd);
  EXPECT_EQ(43, XValueAtEnd);
  EXPECT_EQ(44, YValueAtEnd);
  EXPECT_EQ(45, BarValueAtEnd);
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, LCATest3) {

  LLVMProjectIRDB IRDB({PathToLLFiles + "globals_lca_3.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);

  IDELinearConstantAnalysis Problem(
      &IRDB, &ICFG, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem, &ICFG);

  Solver.solve();

  // Solver.dumpResults();

  const auto *FooInit = IRDB.getInstruction(7);
  // FIXME Why is 10 missing in the results set?
  const auto *BarInit = IRDB.getInstruction(11);
  const auto *LoadX = IRDB.getInstruction(18);
  const auto *LoadY = IRDB.getInstruction(19);
  const auto *End = IRDB.getInstruction(21);
  const auto *Foo = IRDB.getGlobalVariableDefinition("foo");
  const auto *Bar = IRDB.getGlobalVariableDefinition("bar");

  auto FooValueAfterInit = Solver.resultAt(FooInit, Foo);
  auto BarValueAfterInit = Solver.resultAt(BarInit, Bar);

  EXPECT_EQ(42, FooValueAfterInit);
  EXPECT_EQ(45, BarValueAfterInit);

  auto XValueAtEnd = Solver.resultAt(End, LoadX);
  auto FooValueAtEnd = Solver.resultAt(End, Foo);
  auto YValueAtEnd = Solver.resultAt(End, LoadY);
  auto BarValueAtEnd = Solver.resultAt(End, Bar);

  EXPECT_EQ(42, FooValueAtEnd);
  EXPECT_EQ(43, XValueAtEnd);
  EXPECT_EQ(44, YValueAtEnd);
  EXPECT_EQ(45, BarValueAtEnd);
}

// Fails due to exception handling
TEST_F(LLVMBasedICFGGlobCtorDtorTest, DISABLED_LCATest4) {

  LLVMProjectIRDB IRDB({PathToLLFiles + "globals_lca_4.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(
      &IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT, Soundness::Soundy,
      /*IncludeGlobals*/ true); // We have no real global initializers here, but
                                // just keep the flag IncludeGlobals=true
  IDELinearConstantAnalysis Problem(
      &IRDB, &ICFG, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem, &ICFG);

  Solver.solve();

  // Solver.dumpResults();

  const auto *FooGet = IRDB.getInstruction(17);
  const auto *LoadFoo = IRDB.getInstruction(16);
  const auto *LoadX = IRDB.getInstruction(34);
  const auto *End = IRDB.getInstruction(36);

  auto FooValueAfterGet = Solver.resultAt(FooGet, LoadFoo);

  EXPECT_EQ(42, FooValueAfterGet)
      << "Invalid value of " << llvmIRToString(LoadFoo);

  auto XValueAtEnd = Solver.resultAt(End, LoadX);

  EXPECT_EQ(43, XValueAtEnd) << "Invalid value of " << llvmIRToString(LoadX);
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, LCATest4_1) {

  LLVMProjectIRDB IRDB({PathToLLFiles + "globals_lca_4_1.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(
      &IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT, Soundness::Soundy,
      /*IncludeGlobals*/ true); // We have no real global initializers here, but
                                // just keep the flag IncludeGlobals=true
  IDELinearConstantAnalysis Problem(
      &IRDB, &ICFG, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem, &ICFG);

  Solver.solve();

  Solver.dumpResults();

  const auto *FooGet = IRDB.getInstruction(15);
  const auto *LoadFoo = IRDB.getInstruction(14);
  const auto *LoadX = IRDB.getInstruction(20);
  const auto *End = IRDB.getInstruction(22);

  auto FooValueAfterGet = Solver.resultAt(FooGet, LoadFoo);

  EXPECT_EQ(42, FooValueAfterGet)
      << "Invalid value of " << llvmIRToString(LoadFoo);

  auto XValueAtEnd = Solver.resultAt(End, LoadX);

  EXPECT_EQ(43, XValueAtEnd) << "Invalid value of " << llvmIRToString(LoadX);
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, LCATest5) {

  LLVMProjectIRDB IRDB({PathToLLFiles + "globals_lca_5.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy,
                     /*IncludeGlobals*/ true);
  IDELinearConstantAnalysis Problem(
      &IRDB, &ICFG, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem, &ICFG);

  // const auto *GlobalDtor =
  //     ICFG.getRegisteredDtorsCallerOrNull(IRDB.getModule());

  // ASSERT_NE(nullptr, GlobalDtor);

  // GlobalDtor->print(llvm::outs());

  Solver.solve();

  Solver.dumpResults();

  // FIXME: Why is the 27 missing in the results set?
  const auto *AfterGlobalInit = IRDB.getInstruction(4);
  const auto *AtMainPrintF = IRDB.getInstruction(29);

  const auto *Foo = IRDB.getGlobalVariableDefinition("foo");

  EXPECT_EQ(42, Solver.resultAt(AfterGlobalInit, Foo));
  EXPECT_EQ(42, Solver.resultAt(AtMainPrintF, Foo));
  // For whatever reason the analysis computes Top for _this inside the dtor.
  // However, it correctly considers foo=42 inside __PHASAR_DTOR_CALLER.

  // EXPECT_EQ(43, Solver.resultAt(BeforeDtorPrintF, Foo));
}
