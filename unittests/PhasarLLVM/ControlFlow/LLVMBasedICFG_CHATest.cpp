#include <gtest/gtest.h>

#include <string>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>

using namespace std;
using namespace psr;

class LLVMBasedICFG_CHATest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/";
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

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
