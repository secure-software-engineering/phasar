#include <gtest/gtest.h>

#include <string>

#include <llvm/Support/raw_ostream.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

class LLVMBasedICFG_CHATest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedICFG_CHATest, StaticCallSite_1) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/static_callsite_1_c.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *Foo = IRDB.getFunction("foo");
  // iterate all instructions
  for (auto &BB : *F) {
    for (auto &I : BB) {
      // inspect call-sites
      if (llvm::isa<llvm::CallInst>(&I) || llvm::isa<llvm::InvokeInst>(&I)) {
        llvm::ImmutableCallSite CS(&I);
        auto Callees = ICFG.getCalleesOfCallAt(&I);
        ASSERT_EQ(Callees.size(), 1);
        ASSERT_TRUE(Callees.count(Foo));
        ASSERT_TRUE(ICFG.getCallersOf(Foo).count(&I));
      }
    }
  }
}

TEST_F(LLVMBasedICFG_CHATest, VirtualCallSite_2) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/virtual_call_2_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  ASSERT_TRUE(F);

  const llvm::Instruction *I = getNthInstruction(F, 13);
  if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
    llvm::ImmutableCallSite CS(I);
    set<const llvm::Function *> Callees = ICFG.getCalleesOfCallAt(I);
    ASSERT_EQ(Callees.size(), 2);
    set<string> CalleeNames;
    for (const llvm::Function *F : Callees) {
      CalleeNames.insert(F->getName().str());
    }
    ASSERT_TRUE(CalleeNames.count("_ZN1B3fooEv"));
    ASSERT_TRUE(CalleeNames.count("_ZN1A3fooEv"));
  }
}

TEST_F(LLVMBasedICFG_CHATest, VirtualCallSite_9) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/virtual_call_9_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *Foo = IRDB.getFunction("_ZN1D3fooEv");
  ASSERT_TRUE(Foo);
  ASSERT_TRUE(F);

  const llvm::Instruction *I = getNthInstruction(F, 11);
  if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
    llvm::ImmutableCallSite CS(I);
    set<const llvm::Function *> Callees = ICFG.getCalleesOfCallAt(I);
    set<string> CalleeNames;
    for (const llvm::Function *F : Callees) {
      CalleeNames.insert(F->getName().str());
    }
    ASSERT_EQ(Callees.size(), 4);
    ASSERT_TRUE(CalleeNames.count("_ZN1B3fooEv"));
    ASSERT_TRUE(CalleeNames.count("_ZN1A3fooEv"));
    ASSERT_TRUE(CalleeNames.count("_ZN1D3fooEv"));
    ASSERT_TRUE(CalleeNames.count("_ZN1C3fooEv"));
    ASSERT_TRUE(ICFG.getCallersOf(Foo).count(I));
  }
}

TEST_F(LLVMBasedICFG_CHATest, VirtualCallSite_7) {
  ProjectIRDB IRDB({pathToLLFiles + "call_graphs/virtual_call_7_cpp.ll"},
                   IRDBOptions::WPA);
  IRDB.preprocessIR();
  LLVMTypeHierarchy TH(IRDB);
  LLVMBasedICFG ICFG(TH, IRDB, CallGraphAnalysisType::CHA, {"main"});
  llvm::Function *F = IRDB.getFunction("main");
  llvm::Function *VfuncB = IRDB.getFunction("_ZN1B5VfuncEv");
  llvm::Function *VfuncA = IRDB.getFunction("_ZN1A5VfuncEv");
  ASSERT_TRUE(F);
  ASSERT_TRUE(VfuncB);
  ASSERT_TRUE(VfuncA);

  const llvm::Instruction *I = getNthInstruction(F, 19);
  if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
    llvm::ImmutableCallSite CS(I);
    set<const llvm::Function *> Callees = ICFG.getCalleesOfCallAt(I);
    ASSERT_TRUE(ICFG.getCallersOf(VfuncB).count(I));
    ASSERT_EQ(Callees.size(), 2);
    ASSERT_TRUE(Callees.count(VfuncB));
    ASSERT_TRUE(Callees.count(VfuncA));
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
