#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "llvm/Support/raw_ostream.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

TEST(LLVMBasedICFGTest, StaticCallSite_1) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "call_graphs/static_callsite_1_c.ll"},
      IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *Foo = IRDB.getFunctionDefinition("foo");
  ASSERT_TRUE(F);
  ASSERT_TRUE(Foo);
  // iterate all instructions
  for (const auto &BB : *F) {
    for (const auto &I : BB) {
      // inspect call-sites
      if (llvm::isa<llvm::CallInst>(&I) || llvm::isa<llvm::InvokeInst>(&I)) {
        ASSERT_FALSE(ICFG.isVirtualFunctionCall(&I));
      }
    }
  }
}

TEST(LLVMBasedICFGTest, StaticCallSite_2) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "call_graphs/static_callsite_2_c.ll"},
      IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FOO = IRDB.getFunctionDefinition("foo");
  const llvm::Function *BAR = IRDB.getFunctionDefinition("bar");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FOO);
  ASSERT_TRUE(BAR);

  set<const llvm::Function *> FunctionSet;
  FunctionSet.insert(F);
  FunctionSet.insert(FOO);
  FunctionSet.insert(BAR);

  set<const llvm::Function *> FunSet = ICFG.getAllFunctions();
  ASSERT_EQ(FunctionSet, FunSet);

  set<const llvm::Instruction *> CallsFromWithin = ICFG.getCallsFromWithin(F);
  ASSERT_EQ(CallsFromWithin.size(), 2U);
}

TEST(LLVMBasedICFGTest, VirtualCallSite_1) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "call_graphs/virtual_call_1_cpp.ll"},
      IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FooA = IRDB.getFunctionDefinition("_ZN1A3fooEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FooA);

  for (const auto &BB : *F) {
    for (const auto &I : BB) {
      if (llvm::isa<llvm::CallInst>(&I) || llvm::isa<llvm::InvokeInst>(&I)) {
        ASSERT_FALSE(ICFG.isIndirectFunctionCall(&I));
      }
    }
  }
}

TEST(LLVMBasedICFGTest, FunctionPointer_1) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "call_graphs/function_pointer_1_c.ll"},
      IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *Foo = IRDB.getFunctionDefinition("fptr");
  ASSERT_TRUE(F);
  ASSERT_FALSE(Foo);
  for (const auto &BB : *F) {
    for (const auto &I : BB) {
      if (llvm::isa<llvm::CallInst>(&I) || llvm::isa<llvm::InvokeInst>(&I)) {
        ASSERT_TRUE(ICFG.getFunctionOf(&I));
      }
    }
  }
}

TEST(LLVMBasedICFGTest, StaticCallSite_3) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "call_graphs/static_callsite_3_c.ll"},
      IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);
  const llvm::Function *Factorial = IRDB.getFunctionDefinition("factorial");
  ASSERT_TRUE(Factorial);
  for (const auto &BB : *Factorial) {
    for (const auto &I : BB) {
      set<const llvm::Instruction *> CallsFromWithin =
          ICFG.getCallsFromWithin(ICFG.getFunctionOf(&I));
      for (const llvm::Instruction *Inst : CallsFromWithin) {
        std::string MethodName = ICFG.getFunctionName(ICFG.getFunctionOf(Inst));
        ASSERT_EQ(MethodName, "factorial");
      }
    }
  }
}

TEST(LLVMBasedICFGTest, StaticCallSite_4) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "call_graphs/static_callsite_4_cpp.ll"},
      IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  ASSERT_TRUE(F);

  set<const llvm::Function *> CalleesOfCallAt;
  set<const llvm::Function *> CalleesOfCallAtInside;
  int CountFunc = 0;

  for (const auto &BB : *F) {
    for (const auto &I : BB) {
      CalleesOfCallAt = ICFG.getCalleesOfCallAt(&I);
      for (const llvm::Function *Func : CalleesOfCallAt) {
        for (const auto &BB2 : *Func) {
          for (const auto &I2 : BB2) {
            if (llvm::isa<llvm::CallInst>(&I2)) {
              CalleesOfCallAtInside = ICFG.getCalleesOfCallAt(&I2);
              for (const llvm::Function *Func2 : CalleesOfCallAtInside) {
                CountFunc++;
                ASSERT_FALSE(ICFG.isVirtualFunctionCall(&I));
                ASSERT_FALSE(ICFG.isVirtualFunctionCall(&I2));
              }
            }
          }
        }
      }
    }
  }
  ASSERT_EQ(CountFunc, 2);
}

TEST(LLVMBasedICFGTest, StaticCallSite_5) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "call_graphs/static_callsite_5_cpp.ll"},
      IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *Foo =
      IRDB.getFunctionDefinition("_ZN3Foo10getNumFoosEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(Foo);

  for (const auto &BB : *F) {
    for (const auto &I : BB) {
      if (ICFG.isCallStmt(&I)) {
        set<const llvm::Instruction *> CallsFromWithin =
            ICFG.getCallsFromWithin(ICFG.getFunctionOf(&I));
        ASSERT_EQ(CallsFromWithin.size(), 1U);
      }
    }
  }
}

TEST(LLVMBasedICFGTest, StaticCallSite_6) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "call_graphs/static_callsite_6_cpp.ll"},
      IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FooF = IRDB.getFunctionDefinition("_ZN3Foo1fEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FooF);

  const llvm::Instruction *I = getNthInstruction(F, 1);
  if (ICFG.isCallStmt(I) || llvm::isa<llvm::InvokeInst>(I)) {
    set<const llvm::Instruction *> StartPoints = ICFG.getStartPointsOf(FooF);
    set<const llvm::Instruction *> CallsFromWithin =
        ICFG.getCallsFromWithin(ICFG.getFunctionOf(getNthInstruction(F, 2)));

    ASSERT_EQ(StartPoints.size(), 1U);
    ASSERT_TRUE(StartPoints.count(I));
    ASSERT_EQ(CallsFromWithin.size(), 2U);
    ASSERT_TRUE(CallsFromWithin.count(getNthInstruction(F, 2)));
    ASSERT_TRUE(CallsFromWithin.count(getNthInstruction(FooF, 4)));
  }
}

TEST(LLVMBasedICFGTest, StaticCallSite_7) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "call_graphs/static_callsite_7_cpp.ll"},
      IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);
  const llvm::Function *Main = IRDB.getFunctionDefinition("main");
  const llvm::Function *FooF = IRDB.getFunctionDefinition("_ZN3Foo1fEv");
  const llvm::Function *F = IRDB.getFunctionDefinition("_Z1fv");
  ASSERT_TRUE(Main);
  ASSERT_TRUE(FooF);
  ASSERT_TRUE(F);

  const llvm::Instruction *I = getNthInstruction(FooF, 4);
  const llvm::Instruction *LastInst =
      getLastInstructionOf(IRDB.getFunctionDefinition("_ZN3Foo1fEv"));
  set<const llvm::Function *> AllMethods = ICFG.getAllFunctions();
  ASSERT_EQ(LastInst, I);
  ASSERT_EQ(AllMethods.size(), 3U);
  ASSERT_TRUE(AllMethods.count(Main));
  ASSERT_TRUE(AllMethods.count(FooF));
  ASSERT_TRUE(AllMethods.count(F));
}

TEST(LLVMBasedICFGTest, StaticCallSite_8) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "call_graphs/static_callsite_8_cpp.ll"},
      IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::CHA, {"main"}, &TH);
  const llvm::Function *F = IRDB.getFunctionDefinition("main");
  const llvm::Function *FooF = IRDB.getFunctionDefinition("_ZN4Foo21fEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FooF);

  std::vector<const llvm::Instruction *> Insts =
      ICFG.getAllInstructionsOf(FooF);
  std::vector<const llvm::Instruction *> Insts1 =
      ICFG.getAllInstructionsOf(IRDB.getFunctionDefinition("_ZN4Foo21fEv"));
  ASSERT_EQ(Insts.size(), Insts1.size());

  set<const llvm::Function *> FunSet = ICFG.getAllFunctions();
  ASSERT_EQ(FunSet.size(), 3U);

  const llvm::Instruction *I = getNthInstruction(F, 1);
  ASSERT_TRUE(ICFG.isStartPoint(I));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
