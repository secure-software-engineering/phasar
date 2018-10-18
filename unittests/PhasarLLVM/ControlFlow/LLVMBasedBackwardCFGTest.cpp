#include <gtest/gtest.h>
#include <llvm/IR/InstIterator.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

class LLVMBasedBackwardCFGTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedBackwardCFGTest, FallThroughSuccTest) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunction("main");

  ASSERT_EQ(true,false);

}

TEST_F(LLVMBasedBackwardCFGTest, BranchTargetTest) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/switch_cpp.ll"});
  auto F = IRDB.getFunction("main");

  ASSERT_EQ(true,false);
}

TEST_F(LLVMBasedBackwardCFGTest, HandlesMulitplePredeccessors) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunction("main");

  // HANDLING CONDITIONAL BRANCH
  // br i1 %5, label %6, label %9
  auto BRInst = getNthTermInstruction(F, 1);
  std::vector<const llvm::Instruction *> Predeccessors;
  // %7 = load i32, i32* %3, align 4
  Predeccessors.push_back(getNthInstruction(F, 10));
  // %10 = load i32, i32* %3, align 4
  Predeccessors.push_back(getNthInstruction(F, 14));
  auto succsOfBRInst = cfg.getPredsOf(BRInst);
  ASSERT_EQ(succsOfBRInst, Predeccessors);

  // HANDLING UNCONDITIONAL BRANCH
  // br label %12
  BRInst = getNthTermInstruction(F, 3);
  Predeccessors.clear();
  // ret i32 0
  Predeccessors.push_back(getNthTermInstruction(F, 4));
  succsOfBRInst = cfg.getPredsOf(BRInst);
  ASSERT_EQ(succsOfBRInst, Predeccessors);

}

TEST_F(LLVMBasedBackwardCFGTest, HandlesSingleOrEmptyPredeccessor) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/function_call_cpp.ll"});
  auto F = IRDB.getFunction("main");

  // HANDLING SINGLE PREDECCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %4 = call i32 @_Z4multii(i32 2, i32 4)
  auto Pred = getNthInstruction(F, 5);
  std::vector<const llvm::Instruction *> Predeccessor{Pred};
  auto succsOfInst = cfg.getPredsOf(Inst);
  ASSERT_EQ(succsOfInst, Predeccessor);

  // HANDLING EMPTY SUCCESSOR
  // ret i32 0
  auto termInst = getNthTermInstruction(F, 1);
  auto succsOfTermInst = cfg.getPredsOf(termInst);
  Predeccessor.clear();
  ASSERT_EQ(succsOfTermInst, Predeccessor);

  
}

TEST_F(LLVMBasedBackwardCFGTest, HandlesMultipleSuccessors) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunction("main");

  // ret i32 0
  auto TermInst = getNthTermInstruction(F, 4);
  std::vector<const llvm::Instruction *> Successor;
  // br label %12
  Successor.push_back(getNthTermInstruction(F, 2));
  // br label %12
  Successor.push_back(getNthTermInstruction(F, 3));
  auto predsOfTermInst = cfg.getSuccsOf(TermInst);
  ASSERT_EQ(predsOfTermInst, Successor);

}

TEST_F(LLVMBasedBackwardCFGTest, HandlesSingleOrEmptySuccessor) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunction("main");

  // HANDLING SINGLE SUCCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %3 = alloca i32, align 4)
  auto Succ = getNthInstruction(F, 3);
  std::vector<const llvm::Instruction *> Successor{Succ};
  auto predsOfInst = cfg.getSuccsOf(Inst);
  ASSERT_EQ(predsOfInst, Successor);

  // br i1 %11, label %12, label %16
  Inst = getNthTermInstruction(F, 1);
  // %5 = icmp sgt i32 1, %4
  Succ = getNthInstruction(F, 8);
  Successor.clear();
  Successor.push_back(Succ);
  predsOfInst = cfg.getSuccsOf(Inst);
  ASSERT_EQ(predsOfInst, Successor);

  // HANDLING EMPTY SUCCESSOR
  // %1 = alloca i32, align 4
  Inst = getNthInstruction(F, 1);
  predsOfInst = cfg.getSuccsOf(Inst);
  Successor.clear();
  ASSERT_EQ(predsOfInst, Successor);

}

TEST_F(LLVMBasedBackwardCFGTest, HandlesCallSuccessor) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/function_call_cpp.ll"});
  auto F = IRDB.getFunction("main");

  ASSERT_EQ(true,false);

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
