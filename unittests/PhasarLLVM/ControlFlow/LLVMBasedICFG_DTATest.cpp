#include "phasar/Config/Configuration.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace psr;

TEST(LLVMBasedICFG_DTATest, VirtualCallSite_5) {
  GTEST_SKIP() << "Requires typed pointers!";
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/virtual_call_5_cpp.ll");
  DIBasedTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::DTA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FuncA = IRDB.getFunctionDefinition("_ZN1A4funcEv");
  const llvm::Function *VFuncA = IRDB.getFunctionDefinition("_ZN1A5VfuncEv");
  const llvm::Function *VFuncB = IRDB.getFunctionDefinition("_ZN1B5VfuncEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FuncA);
  ASSERT_TRUE(VFuncA);
  ASSERT_TRUE(VFuncB);

  const llvm::Instruction *I = getNthInstruction(F, 16);
  if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
    const auto &Callees = ICFG.getCalleesOfCallAt(I);

    ASSERT_TRUE(ICFG.isVirtualFunctionCall(I));
    ASSERT_EQ(Callees.size(), 2U);
    ASSERT_TRUE(llvm::is_contained(Callees, VFuncB));
    ASSERT_TRUE(llvm::is_contained(Callees, VFuncA));
    ASSERT_TRUE(llvm::is_contained(ICFG.getCallersOf(VFuncA), I));
    ASSERT_TRUE(llvm::is_contained(ICFG.getCallersOf(VFuncB), I));
  }
}

TEST(LLVMBasedICFG_DTATest, VirtualCallSite_6) {
  GTEST_SKIP() << "Requires typed pointers!";
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/virtual_call_6_cpp.ll");
  DIBasedTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::DTA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *VFuncA = IRDB.getFunctionDefinition("_ZN1A5VfuncEv");
  const llvm::Function *VFuncB = IRDB.getFunctionDefinition("_ZN1B5VfuncEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(VFuncA);
  ASSERT_TRUE(VFuncB);

  const llvm::Instruction *I = getNthInstruction(F, 6);
  const auto &Callers = ICFG.getCallersOf(VFuncA);
  ASSERT_EQ(Callers.size(), 1U);
  ASSERT_TRUE(llvm::is_contained(Callers, I));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
