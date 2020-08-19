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

TEST(LLVMBasedICFG_OTFTest, VirtualCallSite_7) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "call_graphs/virtual_call_7_cpp.ll"},
      IRDBOptions::WPA);
  IRDB.emitPreprocessedIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB, false);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT);

  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *VFuncA = IRDB.getFunctionDefinition("_ZN1A5VfuncEv");
  const llvm::Function *VFuncB = IRDB.getFunctionDefinition("_ZN1B5VfuncEv");

  ASSERT_TRUE(F);
  ASSERT_TRUE(VFuncA);
  ASSERT_TRUE(VFuncB);

  const auto *CallToAFunc = getNthInstruction(F, 19);
  ASSERT_TRUE(ICFG.isVirtualFunctionCall(CallToAFunc));
  auto AsCallees = ICFG.getCalleesOfCallAt(CallToAFunc);
  ASSERT_EQ(AsCallees.size(), 2U);
  ASSERT_TRUE(AsCallees.count(VFuncA));
  ASSERT_TRUE(ICFG.getCallersOf(VFuncA).count(CallToAFunc));

  const auto *CallToBFunc = getNthInstruction(F, 25);
  ASSERT_TRUE(ICFG.isVirtualFunctionCall(CallToBFunc));
  auto BsCallees = ICFG.getCalleesOfCallAt(CallToBFunc);
  ASSERT_EQ(BsCallees.size(), 2U);
  ASSERT_TRUE(BsCallees.count(VFuncB));
  ASSERT_TRUE(ICFG.getCallersOf(VFuncB).count(CallToBFunc));
}

// TEST(LLVMBasedICFG_OTFTest, VirtualCallSite_8) {
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

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
