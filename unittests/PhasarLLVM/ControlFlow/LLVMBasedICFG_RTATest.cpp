#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

class LLVMBasedICFG_RTATest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedICFG_RTATest, VirtualCallSite_9) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/virtual_call_9_cpp.ll"},
                   IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::RTA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FooD = IRDB.getFunctionDefinition("_ZN1D3fooEv");
  ASSERT_TRUE(FooD);
  ASSERT_TRUE(F);

  const llvm::Instruction *I = getNthInstruction(F, 11);
  if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
    llvm::ImmutableCallSite CS(I);
    set<const llvm::Function *> Callees = ICFG.getCalleesOfCallAt(I);
    set<string> CalleeNames;
    for (const llvm::Function *F : Callees) {
      CalleeNames.insert(F->getName().str());
    }
    ASSERT_EQ(Callees.size(), 3);
    ASSERT_TRUE(CalleeNames.count("_ZN1B3fooEv"));
    ASSERT_TRUE(CalleeNames.count("_ZN1D3fooEv"));
    ASSERT_TRUE(CalleeNames.count("_ZN1C3fooEv"));
    ASSERT_TRUE(ICFG.getCallersOf(FooD).count(I));
  }
}

TEST_F(LLVMBasedICFG_RTATest, VirtualCallSite_3) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/virtual_call_3_cpp.ll"},
                   IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::RTA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *AptrFoo = IRDB.getFunctionDefinition("_ZN5AImpl3fooEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(AptrFoo);

  const llvm::Instruction *I = getNthInstruction(F, 14);
  if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
    ASSERT_TRUE(ICFG.isVirtualFunctionCall(I));
    std::set<const llvm::Function *> Callees = ICFG.getCalleesOfCallAt(I);
    ASSERT_EQ(Callees.size(), 1);
    ASSERT_TRUE(Callees.count(AptrFoo));
  }
}

TEST_F(LLVMBasedICFG_RTATest, StaticCallSite_13) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/static_callsite_13_cpp.ll"},
                   IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::RTA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *Vfunc = IRDB.getFunctionDefinition("_Z5VfuncP1A");
  const llvm::Function *VfuncA = IRDB.getFunctionDefinition("_ZN1A5VfuncEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(Vfunc);
  ASSERT_TRUE(VfuncA);

  const llvm::Instruction *I = getNthInstruction(F, 15);
  if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
    set<const llvm::Function *> Callees = ICFG.getCalleesOfCallAt(I);
    ASSERT_EQ(Callees.size(), 1);
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
