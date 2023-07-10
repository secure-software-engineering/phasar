#include "phasar/Config/Configuration.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

TEST(LLVMBasedICFG_RTATest, VirtualCallSite_9) {
  LLVMProjectIRDB IRDB({"llvm_test_code/call_graphs/virtual_call_9.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::RTA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FooD = IRDB.getFunctionDefinition("_ZN1D3fooEv");
  ASSERT_TRUE(FooD);
  ASSERT_TRUE(F);

  const llvm::Instruction *I = getNthInstruction(F, 11);
  if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
    const auto &Callees = ICFG.getCalleesOfCallAt(I);
    set<string> CalleeNames;
    for (const llvm::Function *F : Callees) {
      CalleeNames.insert(F->getName().str());
    }
    ASSERT_EQ(Callees.size(), 3U);
    ASSERT_TRUE(CalleeNames.count("_ZN1B3fooEv"));
    ASSERT_TRUE(CalleeNames.count("_ZN1D3fooEv"));
    ASSERT_TRUE(CalleeNames.count("_ZN1C3fooEv"));
    ASSERT_TRUE(llvm::is_contained(ICFG.getCallersOf(FooD), I));
  }
}

TEST(LLVMBasedICFG_RTATest, VirtualCallSite_3) {
  LLVMProjectIRDB IRDB({"llvm_test_code/call_graphs/virtual_call_3.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::RTA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *AptrFoo = IRDB.getFunctionDefinition("_ZN5AImpl3fooEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(AptrFoo);

  const llvm::Instruction *I = getNthInstruction(F, 14);
  if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
    ASSERT_TRUE(ICFG.isVirtualFunctionCall(I));
    const auto &Callees = ICFG.getCalleesOfCallAt(I);
    ASSERT_EQ(Callees.size(), 1U);
    ASSERT_TRUE(llvm::is_contained(Callees, AptrFoo));
  }
}

TEST(LLVMBasedICFG_RTATest, StaticCallSite_13) {
  LLVMProjectIRDB IRDB({"llvm_test_code/call_graphs/static_callsite_13.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::RTA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *Vfunc = IRDB.getFunctionDefinition("_Z5VfuncP1A");
  const llvm::Function *VfuncA = IRDB.getFunctionDefinition("_ZN1A5VfuncEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(Vfunc);
  ASSERT_TRUE(VfuncA);

  const llvm::Instruction *I = getNthInstruction(F, 15);
  if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
    const auto &Callees = ICFG.getCalleesOfCallAt(I);
    ASSERT_EQ(Callees.size(), 1U);
  }
}
