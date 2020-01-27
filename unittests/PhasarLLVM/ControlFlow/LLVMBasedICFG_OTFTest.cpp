#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
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
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToInfo PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *VFuncA = IRDB.getFunctionDefinition("_ZN1A5VfuncEv");
  const llvm::Function *VFuncB = IRDB.getFunctionDefinition("_ZN1B5VfuncEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(VFuncA);
  ASSERT_TRUE(VFuncB);

  set<const llvm::Instruction *> Insts;
  Insts.insert(getNthInstruction(F, 19));
  Insts.insert(getNthInstruction(F, 25));
  for (auto *I : Insts) {
    if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
      set<const llvm::Function *> Callees = ICFG.getCalleesOfCallAt(I);
      ASSERT_TRUE(ICFG.isVirtualFunctionCall(I));
      ASSERT_EQ(Callees.size(), 2);
      ASSERT_TRUE(Callees.count(VFuncB));
      ASSERT_TRUE(Callees.count(VFuncA));
      ASSERT_TRUE(ICFG.getCallersOf(VFuncA).count(I));
      ASSERT_TRUE(ICFG.getCallersOf(VFuncB).count(I));
    }
  }
}

// TEST_F(LLVMBasedICFG_OTFTest, VirtualCallSite_8) {
//   ProjectIRDB IRDB({pathToLLFiles + "call_graphs/virtual_call_8_cpp.ll"},
//                    IRDBOptions::WPA);
//   LLVMTypeHierarchy TH(IRDB);
//   LLVMPointsToInfo PT(IRDB);
//   LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT);
//   const llvm::Function *F = IRDB.getFunctionDefinition("main");
//   const llvm::Function *FooC =
//   IRDB.getFunctionDefinition("_ZZ4mainEN1C3fooEv"); ASSERT_TRUE(F);
//   ASSERT_TRUE(FooC);

//   auto CS1 = getNthInstruction(F, 15);
//   auto CS2 = getNthInstruction(F, 21);

//   auto Callees1 = ICFG.getCalleesOfCallAt(CS1);
//   auto Callees2 = ICFG.getCalleesOfCallAt(CS2);

//   ASSERT_TRUE(Callees1.count(FooC));
//   ASSERT_TRUE(Callees2.count(FooC));
// }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
