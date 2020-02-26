#include <gtest/gtest.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <phasar/Config/Configuration.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

class LLVMBasedBackwardCFGTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedBackwardCFGTest, BranchTargetTest) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");
  auto Term = getNthTermInstruction(F, 1);
  auto a = getNthInstruction(F, 10);
  auto b = getNthInstruction(F, 14);
  auto c = getNthInstruction(F, 12);

  ASSERT_TRUE(cfg.isBranchTarget(a, Term));
  ASSERT_TRUE(cfg.isBranchTarget(b, Term));
  ASSERT_FALSE(cfg.isBranchTarget(c, Term));
}

TEST_F(LLVMBasedBackwardCFGTest, HandlesMulitplePredeccessors) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");

  // HANDLING CONDITIONAL BRANCH
  // br i1 %5, label %6, label %9
  auto BRInst = getNthTermInstruction(F, 1);
  std::vector<const llvm::Instruction *> Predeccessors;
  // %7 = load i32, i32* %3, align 4
  Predeccessors.push_back(getNthInstruction(F, 10));
  // %10 = load i32, i32* %3, align 4
  Predeccessors.push_back(getNthInstruction(F, 14));
  auto predsOfBRInst = cfg.getPredsOf(BRInst);
  ASSERT_EQ(predsOfBRInst, Predeccessors);

  // HANDLING UNCONDITIONAL BRANCH
  // br label %12
  BRInst = getNthTermInstruction(F, 3);
  Predeccessors.clear();
  // ret i32 0
  Predeccessors.push_back(getNthTermInstruction(F, 4));
  predsOfBRInst = cfg.getPredsOf(BRInst);
  ASSERT_EQ(predsOfBRInst, Predeccessors);
}

TEST_F(LLVMBasedBackwardCFGTest, HandlesSingleOrEmptyPredeccessor) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/function_call_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");

  // HANDLING SINGLE PREDECCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %4 = call i32 @_Z4multii(i32 2, i32 4)
  auto Pred = getNthInstruction(F, 5);
  std::vector<const llvm::Instruction *> Predeccessor{Pred};
  auto predsOfInst = cfg.getPredsOf(Inst);
  ASSERT_EQ(predsOfInst, Predeccessor);

  // HANDLING EMPTY SUCCESSOR
  // ret i32 0
  auto termInst = getNthTermInstruction(F, 1);
  auto predsOfTermInst = cfg.getPredsOf(termInst);
  Predeccessor.clear();
  ASSERT_EQ(predsOfTermInst, Predeccessor);
}

TEST_F(LLVMBasedBackwardCFGTest, HandlesMultipleSuccessors) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");

  // ret i32 0
  auto TermInst = getNthTermInstruction(F, 4);
  std::cout << llvmIRToString(TermInst) << std::endl;
  std::vector<const llvm::Instruction *> Successor;
  // br label %12
  Successor.push_back(getNthTermInstruction(F, 3));
  // br label %12
  Successor.push_back(getNthTermInstruction(F, 2));
  auto succsOfTermInst = cfg.getSuccsOf(TermInst);
  ASSERT_EQ(succsOfTermInst, Successor);
}

TEST_F(LLVMBasedBackwardCFGTest, HandlesSingleOrEmptySuccessor) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");

  // HANDLING SINGLE SUCCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %3 = alloca i32, align 4)
  auto Succ = getNthInstruction(F, 3);
  std::vector<const llvm::Instruction *> Successor{Succ};
  auto succsOfInst = cfg.getSuccsOf(Inst);
  ASSERT_EQ(succsOfInst, Successor);

  // br i1 %11, label %12, label %16
  Inst = getNthTermInstruction(F, 1);
  // %5 = icmp sgt i32 1, %4
  Succ = getNthInstruction(F, 8);
  Successor.clear();
  Successor.push_back(Succ);
  succsOfInst = cfg.getSuccsOf(Inst);
  ASSERT_EQ(succsOfInst, Successor);

  // HANDLING EMPTY SUCCESSOR
  // %1 = alloca i32, align 4
  Inst = getNthInstruction(F, 1);
  succsOfInst = cfg.getSuccsOf(Inst);
  Successor.clear();
  ASSERT_EQ(succsOfInst, Successor);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
