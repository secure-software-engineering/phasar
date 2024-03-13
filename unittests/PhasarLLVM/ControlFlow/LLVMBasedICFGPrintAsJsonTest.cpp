#include "phasar/Config/Configuration.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <string>
#include <vector>

using namespace std;
using namespace psr;

template <typename T> static auto makeSet(T &&Vec) {
  using value_type = std::decay_t<decltype(*Vec.begin())>;
  return std::set<value_type>{Vec.begin(), Vec.end()};
}

TEST(LLVMBasedICFGTest, StaticCallSite_1) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/static_callsite_1_c.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *Foo = IRDB.getFunctionDefinition("foo");
  ASSERT_TRUE(F);
  ASSERT_TRUE(Foo);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, StaticCallSite_2a) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/static_callsite_2_c.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT,
                     Soundness::Soundy, false);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FOO = IRDB.getFunctionDefinition("foo");
  const llvm::Function *BAR = IRDB.getFunctionDefinition("bar");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FOO);
  ASSERT_TRUE(BAR);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, StaticCallSite_2b) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/static_callsite_2_c.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FOO = IRDB.getFunctionDefinition("foo");
  const llvm::Function *BAR = IRDB.getFunctionDefinition("bar");
  const llvm::Function *CTOR =
      IRDB.getFunctionDefinition(LLVMBasedICFG::GlobalCRuntimeModelName);
  const llvm::Function *DTOR =
      IRDB.getFunctionDefinition(LLVMBasedICFG::GlobalCRuntimeDtorModelName);
  ASSERT_TRUE(F);
  ASSERT_TRUE(FOO);
  ASSERT_TRUE(BAR);
  ASSERT_TRUE(CTOR);
  ASSERT_TRUE(DTOR);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, VirtualCallSite_1) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/virtual_call_1_cpp.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FooA = IRDB.getFunctionDefinition("_ZN1A3fooEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FooA);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, FunctionPointer_1) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/function_pointer_1_c.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *Foo = IRDB.getFunctionDefinition("fptr");
  ASSERT_TRUE(F);
  ASSERT_FALSE(Foo);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, StaticCallSite_3) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/static_callsite_3_c.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT);
  const llvm::Function *Factorial = IRDB.getFunctionDefinition("factorial");
  ASSERT_TRUE(Factorial);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, StaticCallSite_4) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/static_callsite_4_cpp.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  ASSERT_TRUE(F);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, StaticCallSite_5) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/static_callsite_5_cpp.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *Foo =
      IRDB.getFunctionDefinition("_ZN3Foo10getNumFoosEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(Foo);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, StaticCallSite_6) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/static_callsite_6_cpp.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FooF = IRDB.getFunctionDefinition("_ZN3Foo1fEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FooF);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, StaticCallSite_7) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/static_callsite_7_cpp.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT);
  const llvm::Function *Main = IRDB.getFunctionDefinition("main");
  const llvm::Function *FooF = IRDB.getFunctionDefinition("_ZN3Foo1fEv");
  const llvm::Function *F = IRDB.getFunctionDefinition("_Z1fv");
  ASSERT_TRUE(Main);
  ASSERT_TRUE(FooF);
  ASSERT_TRUE(F);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, StaticCallSite_8) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/static_callsite_8_cpp.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FooF = IRDB.getFunctionDefinition("_ZN4Foo21fEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FooF);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, GlobalCtorDtor_1) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/global_ctor_dtor_1_cpp.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT,
                     Soundness::Soundy, true);

  auto *GlobCtorFn = IRDB.getFunction(LLVMBasedICFG::GlobalCRuntimeModelName);

  ASSERT_TRUE(GlobCtorFn);

  // GlobCtorFn->print(llvm::outs());

  const llvm::Function *Main = IRDB.getFunctionDefinition("main");
  const llvm::Function *BeforeMain =
      IRDB.getFunctionDefinition("_Z11before_mainv");

  ASSERT_TRUE(Main);
  ASSERT_TRUE(BeforeMain);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, GlobalCtorDtor_2) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/global_ctor_dtor_2_cpp.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT,
                     Soundness::Soundy, true);
  const llvm::Function *Main = IRDB.getFunctionDefinition("main");
  const llvm::Function *BeforeMain =
      IRDB.getFunctionDefinition("_Z11before_mainv");
  const llvm::Function *AfterMain =
      IRDB.getFunctionDefinition("_Z10after_mainv");

  ASSERT_TRUE(Main);
  ASSERT_TRUE(BeforeMain);
  ASSERT_TRUE(AfterMain);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, GlobalCtorDtor_3) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/global_ctor_dtor_3_cpp.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT,
                     Soundness::Soundy, true);
  const llvm::Function *Main = IRDB.getFunctionDefinition("main");
  const llvm::Function *Ctor = IRDB.getFunctionDefinition("_ZN1SC2Ei");
  const llvm::Function *Dtor = IRDB.getFunctionDefinition("_ZN1SD2Ev");

  ASSERT_TRUE(Main);
  ASSERT_TRUE(Ctor);
  ASSERT_TRUE(Dtor);

  ICFG.printAsJson();
}

TEST(LLVMBasedICFGTest, GlobalCtorDtor_4) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/global_ctor_dtor_4_cpp.ll");
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH, &PT,
                     Soundness::Soundy, true);
  const llvm::Function *Main = IRDB.getFunctionDefinition("main");
  const llvm::Function *Ctor = IRDB.getFunctionDefinition("_ZN1SC2Ei");
  const llvm::Function *Dtor = IRDB.getFunctionDefinition("_ZN1SD2Ev");
  const llvm::Function *BeforeMain =
      IRDB.getFunctionDefinition("_Z11before_mainv");
  const llvm::Function *AfterMain =
      IRDB.getFunctionDefinition("_Z10after_mainv");

  ASSERT_TRUE(Main);
  ASSERT_TRUE(Ctor);
  ASSERT_TRUE(Dtor);
  ASSERT_TRUE(BeforeMain);
  ASSERT_TRUE(AfterMain);

  ICFG.printAsJson();
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
