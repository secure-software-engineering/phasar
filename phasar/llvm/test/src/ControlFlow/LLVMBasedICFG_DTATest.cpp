#include "gtest/gtest.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

TEST(LLVMBasedICFG_DTATest, VirtualCallSite_5) {
  ProjectIRDB IRDB({"llvm_test_code/call_graphs/virtual_call_5.ll"},
                   IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::DTA, {"main"}, &TH, &PT);
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
    set<const llvm::Function *> Callees = ICFG.getCalleesOfCallAt(I);

    ASSERT_TRUE(ICFG.isVirtualFunctionCall(I));
    ASSERT_EQ(Callees.size(), 2U);
    ASSERT_TRUE(Callees.count(VFuncB));
    ASSERT_TRUE(Callees.count(VFuncA));
    ASSERT_TRUE(ICFG.getCallersOf(VFuncA).count(I));
    ASSERT_TRUE(ICFG.getCallersOf(VFuncB).count(I));
  }
}

TEST(LLVMBasedICFG_DTATest, VirtualCallSite_6) {
  ProjectIRDB IRDB({"llvm_test_code/call_graphs/virtual_call_6.ll"},
                   IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::DTA, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *VFuncA = IRDB.getFunctionDefinition("_ZN1A5VfuncEv");
  const llvm::Function *VFuncB = IRDB.getFunctionDefinition("_ZN1B5VfuncEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(VFuncA);
  ASSERT_TRUE(VFuncB);

  const llvm::Instruction *I = getNthInstruction(F, 6);
  set<const llvm::Instruction *> Callers = ICFG.getCallersOf(VFuncA);
  ASSERT_EQ(Callers.size(), 1U);
  ASSERT_TRUE(Callers.count(I));
}
