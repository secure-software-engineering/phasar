#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

class LLVMBasedICFG_OTFTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedICFG_OTFTest, VirtualCallSite_7) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/virtual_call_7_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::OTF, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *VFuncA = IRDB.getFunction("_ZN1A5VfuncEv");
  llvm::Function *VFuncB = IRDB.getFunction("_ZN1B5VfuncEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(VFuncA);
  ASSERT_TRUE(VFuncB);

  set<const llvm::Instruction *> Insts;
  Insts.insert(getNthInstruction(F, 19));
  Insts.insert(getNthInstruction(F, 25));
  for (auto *I : Insts) {
    if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
      llvm::ImmutableCallSite CS(I);
      set<const llvm::Function *> Callees = ICFG.getCalleesOfCallAt(I);
      ASSERT_TRUE(ICFG.isVirtualFunctionCall(CS));
      ASSERT_EQ(Callees.size(), 2);
      ASSERT_TRUE(Callees.count(VFuncB));
      ASSERT_TRUE(Callees.count(VFuncA));
      ASSERT_TRUE(ICFG.getCallersOf(VFuncA).count(I));
      ASSERT_TRUE(ICFG.getCallersOf(VFuncB).count(I));
    }
  }
}

TEST_F(LLVMBasedICFG_OTFTest, VirtualCallSite_8) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/virtual_call_8_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::OTF, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *FooC = IRDB.getFunction("_ZZ4mainEN1C3fooEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FooC);

  set<const llvm::Instruction *> Insts;
  Insts.insert(getNthInstruction(F, 15));
  Insts.insert(getNthInstruction(F, 21));
  for (auto *I : Insts) {
    llvm::ImmutableCallSite CS(I);
    set<const llvm::Function *> Callees = ICFG.getCalleesOfCallAt(I);
    ASSERT_EQ(Callees.size(), 1);
    ASSERT_TRUE(Callees.count(FooC));
    ASSERT_TRUE(ICFG.getCallersOf(FooC).count(I));
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
