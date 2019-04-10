#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <llvm/Support/raw_ostream.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

class LLVMBasedICFGTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedICFGTest, StaticCallSite_1) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/static_callsite_1_c.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *Foo = IRDB.getFunction("foo");
  ASSERT_TRUE(F);
  ASSERT_TRUE(Foo);
  // iterate all instructions
  for (auto &BB : *F) {
    for (auto &I : BB) {
      // inspect call-sites
      if (llvm::isa<llvm::CallInst>(&I) || llvm::isa<llvm::InvokeInst>(&I)) {
        llvm::ImmutableCallSite CS(&I);
        ASSERT_FALSE(ICFG.isVirtualFunctionCall(CS));
      }
    }
  }
}

TEST_F(LLVMBasedICFGTest, StaticCallSite_2) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/static_callsite_2_c.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *FOO = IRDB.getFunction("foo");
  llvm::Function *BAR = IRDB.getFunction("bar");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FOO);
  ASSERT_TRUE(BAR);

  set<const llvm::Function *> FunctionSet;
  FunctionSet.insert(F);
  FunctionSet.insert(FOO);
  FunctionSet.insert(BAR);

  set<const llvm::Function *> FunSet = ICFG.getAllMethods();
  ASSERT_EQ(FunctionSet, FunSet);

  set<const llvm::Instruction *> callsFromWithin = ICFG.getCallsFromWithin(F);
  ASSERT_EQ(2, callsFromWithin.size());
}

TEST_F(LLVMBasedICFGTest, VirtualCallSite_1) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/virtual_call_1_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *FooA = IRDB.getFunction("_ZN1A3fooEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FooA);

  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (llvm::isa<llvm::CallInst>(&I) || llvm::isa<llvm::InvokeInst>(&I)) {
        llvm::ImmutableCallSite CS(&I);
        ASSERT_TRUE(ICFG.isVirtualFunctionCall(CS));
      }
    }
  }
}

TEST_F(LLVMBasedICFGTest, FunctionPointer_1) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/function_pointer_1_c.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *Foo = IRDB.getFunction("fptr");
  ASSERT_TRUE(F);
  ASSERT_FALSE(Foo);
  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (llvm::isa<llvm::CallInst>(&I) || llvm::isa<llvm::InvokeInst>(&I)) {
        ASSERT_TRUE(ICFG.getMethodOf(&I));
      }
    }
  }
}

TEST_F(LLVMBasedICFGTest, StaticCallSite_3) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/static_callsite_3_c.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *Factorial = IRDB.getFunction("factorial");
  ASSERT_TRUE(Factorial);
  for (auto &BB : *Factorial) {
    for (auto &I : BB) {
      set<const llvm::Instruction *> callsFromWithin =
          ICFG.getCallsFromWithin(ICFG.getMethodOf(&I));
      for (const llvm::Instruction *Inst : callsFromWithin) {
        std::string methodName = ICFG.getMethodName(ICFG.getMethodOf(Inst));
        ASSERT_EQ(methodName, "factorial");
      }
    }
  }
}

TEST_F(LLVMBasedICFGTest, StaticCallSite_4) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/static_callsite_4_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  ASSERT_TRUE(F);

  set<const llvm::Function *> calleesOfCallAt;
  set<const llvm::Function *> calleesOfCallAtInside;
  int countFunc = 0;

  for (auto &BB : *F) {
    for (auto &I : BB) {
      calleesOfCallAt = ICFG.getCalleesOfCallAt(&I);
      for (const llvm::Function *Func : calleesOfCallAt) {
        for (auto &BB2 : *Func) {
          for (auto &I2 : BB2) {
            if (llvm::isa<llvm::CallInst>(&I2)) {
              calleesOfCallAtInside = ICFG.getCalleesOfCallAt(&I2);
              for (const llvm::Function *Func2 : calleesOfCallAtInside) {
                countFunc++;
                llvm::ImmutableCallSite CS(&I);
                ASSERT_FALSE(ICFG.isVirtualFunctionCall(CS));
                llvm::ImmutableCallSite CS2(&I2);
                ASSERT_FALSE(ICFG.isVirtualFunctionCall(CS2));
              }
            }
          }
        }
      }
    }
  }
  ASSERT_EQ(countFunc, 2);
}

TEST_F(LLVMBasedICFGTest, StaticCallSite_5) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/static_callsite_5_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *Foo = IRDB.getFunction("_ZN3Foo10getNumFoosEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(Foo);

  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (ICFG.isCallStmt(&I)) {
        set<const llvm::Instruction *> callsFromWithin =
            ICFG.getCallsFromWithin(ICFG.getMethodOf(&I));
        ASSERT_EQ(callsFromWithin.size(), 1);
      }
    }
  }
}

TEST_F(LLVMBasedICFGTest, StaticCallSite_6) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/static_callsite_6_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *FooF = IRDB.getFunction("_ZN3Foo1fEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FooF);

  const llvm::Instruction *I = getNthInstruction(F, 1);
  if (ICFG.isCallStmt(I) || llvm::isa<llvm::InvokeInst>(I)) {
    set<const llvm::Instruction *> startPoints = ICFG.getStartPointsOf(FooF);
    set<const llvm::Instruction *> callsFromWithin =
        ICFG.getCallsFromWithin(ICFG.getMethodOf(getNthInstruction(F, 2)));

    ASSERT_EQ(startPoints.size(), 1);
    ASSERT_TRUE(startPoints.count(I));
    ASSERT_EQ(callsFromWithin.size(), 2);
    ASSERT_TRUE(callsFromWithin.count(getNthInstruction(F, 2)));
    ASSERT_TRUE(callsFromWithin.count(getNthInstruction(FooF, 4)));
  }
}

TEST_F(LLVMBasedICFGTest, StaticCallSite_7) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/static_callsite_7_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *Main = IRDB.getFunction("main");
  llvm::Function *FooF = IRDB.getFunction("_ZN3Foo1fEv");
  llvm::Function *F = IRDB.getFunction("_Z1fv");
  ASSERT_TRUE(Main);
  ASSERT_TRUE(FooF);
  ASSERT_TRUE(F);

  const llvm::Instruction *I = getNthInstruction(FooF, 4);
  const llvm::Instruction *lastInst = ICFG.getLastInstructionOf("_ZN3Foo1fEv");
  set<const llvm::Function *> allMethods = ICFG.getAllMethods();
  ASSERT_EQ(lastInst, I);
  ASSERT_EQ(allMethods.size(), 3);
  ASSERT_TRUE(allMethods.count(Main));
  ASSERT_TRUE(allMethods.count(FooF));
  ASSERT_TRUE(allMethods.count(F));
}

TEST_F(LLVMBasedICFGTest, StaticCallSite_8) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/static_callsite_8_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *FooF = IRDB.getFunction("_ZN4Foo21fEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FooF);

  std::vector<const llvm::Instruction *> Insts =
      ICFG.getAllInstructionsOf(FooF);
  std::vector<const llvm::Instruction *> Insts1 =
      ICFG.getAllInstructionsOfFunction("_ZN4Foo21fEv");
  ASSERT_EQ(Insts.size(), Insts1.size());

  set<const llvm::Function *> FunSet = ICFG.getAllMethods();
  ASSERT_EQ(FunSet.size(), 3);

  const llvm::Instruction *I = getNthInstruction(F, 1);
  ASSERT_TRUE(ICFG.isStartPoint(I));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}