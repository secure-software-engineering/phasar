

#include "phasar/Config/Configuration.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

TEST(LLVMBasedICFG_OTFTest, VirtualCallSite_7) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/virtual_call_7_cpp_dbg.ll");
  DIBasedTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB, false);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT);

  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *VFuncA = IRDB.getFunctionDefinition("_ZN1A5VfuncEv");
  const llvm::Function *VFuncB = IRDB.getFunctionDefinition("_ZN1B5VfuncEv");

  ASSERT_TRUE(F);
  ASSERT_TRUE(VFuncA);
  ASSERT_TRUE(VFuncB);

  const auto *CallToAFunc = getNthInstruction(F, 17);
  ASSERT_TRUE(ICFG.isVirtualFunctionCall(CallToAFunc));
  const auto &AsCallees = ICFG.getCalleesOfCallAt(CallToAFunc);
  ASSERT_EQ(AsCallees.size(), 2U);
  ASSERT_TRUE(llvm::is_contained(AsCallees, VFuncA));
  ASSERT_TRUE(llvm::is_contained(ICFG.getCallersOf(VFuncA), CallToAFunc));

  const auto *CallToBFunc = getNthInstruction(F, 22);
  ASSERT_TRUE(ICFG.isVirtualFunctionCall(CallToBFunc));
  const auto &BsCallees = ICFG.getCalleesOfCallAt(CallToBFunc);
  ASSERT_EQ(BsCallees.size(), 2U);

  ASSERT_TRUE(llvm::is_contained(BsCallees, VFuncB));
  ASSERT_TRUE(llvm::is_contained(ICFG.getCallersOf(VFuncB), CallToBFunc));
}

// TEST(LLVMBasedICFG_OTFTest, VirtualCallSite_8) {
//   ProjectIRDB IRDB({pathToLLFiles + "call_graphs/virtual_call_8_cpp.ll"},
//                    IRDBOptions::WPA);
//   DIBasedTypeHierarchy TH(IRDB);
//   LLVMAliasInfo PT(IRDB);
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

TEST(LLVMBasedICFG_OTFTest, FunctionPtrCall_2) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/function_pointer_2_cpp_dbg.ll");
  DIBasedTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB, false);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT);

  const llvm::Function *Main = IRDB.getFunctionDefinition("main");
  const llvm::Function *Bar = IRDB.getFunctionDefinition("_Z3barv");

  const auto *FPtrCall = getNthInstruction(Main, 9);
  const auto &Callees = ICFG.getCalleesOfCallAt(FPtrCall);

  auto printCallees // NOLINT
      = [&]() {
          std::string Ret;
          llvm::raw_string_ostream OS(Ret);

          OS << "{ ";
          bool First = true;
          for (const auto *Callee : Callees) {
            if (First) {
              First = false;
            } else {
              OS << ", ";
            }

            OS << Callee->getName();
          }

          OS << " }";
          return Ret;
        };

  ASSERT_EQ(Callees.size(), 1U) << "Too many callees: " << printCallees();
  ASSERT_EQ(llvm::is_contained(Callees, Bar), 1U);
}

TEST(LLVMBasedICFG_OTFTest, FunctionPtrCall_3) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "call_graphs/function_pointer_3_cpp_dbg.ll");
  DIBasedTypeHierarchy TH(IRDB);
  LLVMAliasSet PT(&IRDB, false);
  LLVMBasedICFG ICFG(&IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT);

  const llvm::Function *Main = IRDB.getFunctionDefinition("main");
  const llvm::Function *Foo = IRDB.getFunctionDefinition("_Z3foov");

  const auto *FPtrCall = getNthInstruction(Main, 10);

  const auto &Callees = ICFG.getCalleesOfCallAt(FPtrCall);

  ASSERT_EQ(Callees.size(), 1U);
  ASSERT_EQ(llvm::is_contained(Callees, Foo), 1U);
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
