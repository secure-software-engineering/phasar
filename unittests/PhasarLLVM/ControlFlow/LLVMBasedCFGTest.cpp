#include <gtest/gtest.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <phasar/Config/Configuration.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

class LLVMBasedCFGTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedCFGTest, FallThroughSuccTest) {
  LLVMBasedCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");

  // HANDLING CONDITIONAL BRANCH
  // br i1 %5, label %6, label %9
  auto BranchInst = getNthTermInstruction(F, 1);
  // %7 = load i32, i32* %3, align 4
  ASSERT_FALSE(
      cfg.isFallThroughSuccessor(BranchInst, getNthInstruction(F, 10)));
  // %10 = load i32, i32* %3, align 4
  ASSERT_TRUE(cfg.isFallThroughSuccessor(BranchInst, getNthInstruction(F, 14)));

  // HANDLING UNCONDITIONAL BRANCH
  // br label %12
  BranchInst = getNthTermInstruction(F, 2);
  // ret i32 0
  ASSERT_TRUE(
      cfg.isFallThroughSuccessor(BranchInst, getNthTermInstruction(F, 4)));
}

TEST_F(LLVMBasedCFGTest, BranchTargetTest) {
  LLVMBasedCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/switch_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");

  // HANDLING SWITCH INSTRUCTION
  // switch i32 %4, label %8 [
  //   i32 65, label %5
  //   i32 66, label %6
  //   i32 67, label %6
  //   i32 68, label %7
  // ]
  auto SwitchInst = getNthTermInstruction(F, 1);
  // store i32 0, i32* %2, align 4
  ASSERT_FALSE(cfg.isBranchTarget(SwitchInst, getNthStoreInstruction(F, 2)));
  // store i32 10, i32* %2, align 4
  ASSERT_TRUE(cfg.isBranchTarget(SwitchInst, getNthStoreInstruction(F, 3)));
  // store i32 20, i32* %2, align 4
  ASSERT_TRUE(cfg.isBranchTarget(SwitchInst, getNthStoreInstruction(F, 4)));
  // store i32 30, i32* %2, align 4
  ASSERT_TRUE(cfg.isBranchTarget(SwitchInst, getNthStoreInstruction(F, 5)));
  // store i32 -1, i32* %2, align 4
  ASSERT_TRUE(cfg.isBranchTarget(SwitchInst, getNthStoreInstruction(F, 6)));
  // ret i32 0
  ASSERT_FALSE(cfg.isBranchTarget(SwitchInst, getNthTermInstruction(F, 6)));

  // HANDLING BRANCH INSTRUCTION
  // br label %9
  auto BranchInst = getNthTermInstruction(F, 2);
  // store i32 20, i32* %2, align 4
  ASSERT_FALSE(cfg.isBranchTarget(BranchInst, getNthStoreInstruction(F, 4)));
  // ret i32 0
  ASSERT_TRUE(cfg.isBranchTarget(BranchInst, getNthTermInstruction(F, 6)));
}

TEST_F(LLVMBasedCFGTest, HandlesMulitplePredeccessors) {
  LLVMBasedCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");

  // ret i32 0
  auto TermInst = getNthTermInstruction(F, 4);
  std::vector<const llvm::Instruction *> Predeccessor;
  // br label %12
  Predeccessor.push_back(getNthTermInstruction(F, 2));
  // br label %12
  Predeccessor.push_back(getNthTermInstruction(F, 3));
  auto predsOfTermInst = cfg.getPredsOf(TermInst);
  ASSERT_EQ(predsOfTermInst, Predeccessor);
}

TEST_F(LLVMBasedCFGTest, HandlesSingleOrEmptyPredeccessor) {
  LLVMBasedCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");

  // HANDLING SINGLE PREDECCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %3 = alloca i32, align 4)
  auto Pred = getNthInstruction(F, 3);
  std::vector<const llvm::Instruction *> Predeccessor{Pred};
  auto predsOfInst = cfg.getPredsOf(Inst);
  ASSERT_EQ(predsOfInst, Predeccessor);

  // br i1 %11, label %12, label %16
  Inst = getNthTermInstruction(F, 1);
  // %5 = icmp sgt i32 1, %4
  Pred = getNthInstruction(F, 8);
  Predeccessor.clear();
  Predeccessor.push_back(Pred);
  predsOfInst = cfg.getPredsOf(Inst);
  ASSERT_EQ(predsOfInst, Predeccessor);

  // HANDLING EMPTY PREDECCESSOR
  // %1 = alloca i32, align 4
  Inst = getNthInstruction(F, 1);
  predsOfInst = cfg.getPredsOf(Inst);
  Predeccessor.clear();
  ASSERT_EQ(predsOfInst, Predeccessor);
}

TEST_F(LLVMBasedCFGTest, HandlesMultipleSuccessors) {
  LLVMBasedCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");

  // HANDLING CONDITIONAL BRANCH
  // br i1 %5, label %6, label %9
  auto BRInst = getNthTermInstruction(F, 1);
  std::vector<const llvm::Instruction *> Successors;
  // %7 = load i32, i32* %3, align 4
  Successors.push_back(getNthInstruction(F, 10));
  // %10 = load i32, i32* %3, align 4
  Successors.push_back(getNthInstruction(F, 14));
  auto succsOfBRInst = cfg.getSuccsOf(BRInst);
  ASSERT_EQ(succsOfBRInst, Successors);

  // HANDLING UNCONDITIONAL BRANCH
  // br label %12
  BRInst = getNthTermInstruction(F, 3);
  Successors.clear();
  // ret i32 0
  Successors.push_back(getNthTermInstruction(F, 4));
  succsOfBRInst = cfg.getSuccsOf(BRInst);
  ASSERT_EQ(succsOfBRInst, Successors);
}

TEST_F(LLVMBasedCFGTest, HandlesSingleOrEmptySuccessor) {
  LLVMBasedCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/function_call_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");

  // HANDLING SINGLE SUCCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %4 = call i32 @_Z4multii(i32 2, i32 4)
  auto Succ = getNthInstruction(F, 5);
  std::vector<const llvm::Instruction *> Successors{Succ};
  auto succsOfInst = cfg.getSuccsOf(Inst);
  ASSERT_EQ(succsOfInst, Successors);

  // HANDLING EMPTY SUCCESSOR
  // ret i32 0
  auto termInst = getNthTermInstruction(F, 1);
  auto succsOfTermInst = cfg.getSuccsOf(termInst);
  Successors.clear();
  ASSERT_EQ(succsOfTermInst, Successors);
}

TEST_F(LLVMBasedCFGTest, HandlesCallSuccessor) {
  LLVMBasedCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/function_call_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");

  // HANDLING CALL INSTRUCTION SUCCESSOR
  // %4 = call i32 @_Z4multii(i32 2, i32 4)
  auto callInst = getNthInstruction(F, 5);
  // store i32 %4, i32* %2, align 4
  auto Succ = getNthStoreInstruction(F, 2);
  auto succsOfCallInst = cfg.getSuccsOf(callInst);
  std::vector<const llvm::Instruction *> Successors{Succ};
  ASSERT_EQ(succsOfCallInst, Successors);
}

TEST_F(LLVMBasedCFGTest, HandleFieldLoadsArray) {
  LLVMBasedCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "fields/array_1_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");
  auto Inst = getNthInstruction(F, 1);
  ASSERT_FALSE(cfg.isFieldLoad(Inst));
  Inst = getNthInstruction(F, 6);
  ASSERT_TRUE(cfg.isFieldLoad(Inst));
}

TEST_F(LLVMBasedCFGTest, HandleFieldStoreArray) {
  LLVMBasedCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "fields/array_1_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");
  auto Inst = getNthInstruction(F, 1);
  ASSERT_FALSE(cfg.isFieldStore(Inst));
  Inst = getNthInstruction(F, 9);
  ASSERT_TRUE(cfg.isFieldStore(Inst));
}

TEST_F(LLVMBasedCFGTest, HandleFieldLoadsField) {
  LLVMBasedCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "fields/field_1_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");
  auto Inst = getNthInstruction(F, 1);
  ASSERT_FALSE(cfg.isFieldLoad(Inst));
  Inst = getNthInstruction(F, 11);
  ASSERT_TRUE(cfg.isFieldLoad(Inst));
  Inst = getNthInstruction(F, 15);
  ASSERT_TRUE(cfg.isFieldLoad(Inst));
  Inst = getNthInstruction(F, 19);
  ASSERT_TRUE(cfg.isFieldLoad(Inst));
}

TEST_F(LLVMBasedCFGTest, HandleFieldStoreField) {
  LLVMBasedCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "fields/field_1_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");
  auto Inst = getNthInstruction(F, 1);
  ASSERT_FALSE(cfg.isFieldStore(Inst));
  Inst = getNthInstruction(F, 5);
  ASSERT_TRUE(cfg.isFieldStore(Inst));
  Inst = getNthInstruction(F, 7);
  ASSERT_TRUE(cfg.isFieldStore(Inst));
  Inst = getNthInstruction(F, 9);
  ASSERT_TRUE(cfg.isFieldStore(Inst));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
