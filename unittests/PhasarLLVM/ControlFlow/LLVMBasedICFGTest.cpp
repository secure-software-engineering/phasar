#include <gtest/gtest.h>

#include <string>

#include <phasar/DB/ProjectIRDB.h>
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

// TEST_F(LLVMBasedICFGTest, StaticCallSite_2) {
//   ProjectIRDB IRDB({pathToLLFiles + "call_graphs/static_callsite_2_c.ll"},
//   IRDBOptions::WPA); IRDB.preprocessIR(); LLVMTypeHierarchy TH(IRDB);
//   LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
//   llvm::Function *F = IRDB.getFunction("main");
//   llvm::Function *FOO = IRDB.getFunction("foo");
//   llvm::Function *BAR = IRDB.getFunction("bar");
//   ASSERT_TRUE(F);
//   ASSERT_TRUE(FOO);
//   ASSERT_TRUE(BAR);

//   set<const llvm::Function *> FunctionSet;
//   FunctionSet.insert(FOO);
//   FunctionSet.insert(BAR);

//   set<const llvm::Function *> FunSet = ICFG.getAllMethods();
//   ASSERT_EQ(FunctionSet , FunSet);
// }

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
        // const llvm::Function *Func = ICFG.getMethodOf(&I);
        ASSERT_TRUE(ICFG.getMethodOf(&I));
        // ASSERT_EQ(Func, Foo);
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
              // calleesOfCallAtInside.insert(ICFG.getCalleesOfCallAt(&I2));
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

TEST_F(LLVMBasedICFGTest, VirtualCallSite_2) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/virtual_call_2_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *FooB = IRDB.getFunction("_ZN1BC2Ev");
  ASSERT_TRUE(F);
  ASSERT_TRUE(FooB);

  for (auto &BB : *F) {
    for (auto &I : BB) {
      set<const llvm::Function *> CalleesOfCallAt = ICFG.getCalleesOfCallAt(&I);
      if (llvm::isa<llvm::CallInst>(&I) || llvm::isa<llvm::InvokeInst>(&I)) {
        llvm::ImmutableCallSite CS(&I);
        if (ICFG.isVirtualFunctionCall(CS)) {
          for (const llvm::Function *Func : CalleesOfCallAt) {
            std::string methodName = ICFG.getMethodName(Func);
            // ASSERT_EQ(methodName, "_ZN1BC2Ev");
            // cout << "Name: " << methodName
            //      << " Size of CalleesOfCallAt: " << CalleesOfCallAt.size()
            //      << endl;

            ASSERT_TRUE(CalleesOfCallAt.size());
          }
        }
      }
    }
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
