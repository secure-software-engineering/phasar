#include <gtest/gtest.h>
#include <llvm/IR/InstIterator.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>

using namespace psr;

TEST(LLVMBasedCFGTest, FallThroughSuccTest) {
  // LLVMBasedCFG cfg;
  // ProjectIRDB IRDB({"../../../../test/llvm_test_code/control_flow/branch.ll"});
  // auto F = IRDB.getFunction("main");
  //
  // // HANDLING CONDITIONAL BRANCH
  // // br i1 %5, label %6, label %9
  // auto BranchInst = getNthTermInstruction(F, 1);
  // // %7 = load i32, i32* %3, align 4
  // ASSERT_FALSE(
  //     cfg.isFallThroughSuccessor(BranchInst, getNthInstruction(F, 10)));
  // // %10 = load i32, i32* %3, align 4
  // ASSERT_TRUE(cfg.isFallThroughSuccessor(BranchInst, getNthInstruction(F, 14)));
  //
  // // HANDLING UNCONDITIONAL BRANCH
  // // br label %12
  // BranchInst = getNthTermInstruction(F, 2);
  // // ret i32 0
  // ASSERT_TRUE(
  //     cfg.isFallThroughSuccessor(BranchInst, getNthTermInstruction(F, 4)));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
