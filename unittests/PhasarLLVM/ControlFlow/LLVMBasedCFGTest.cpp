#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

class LLVMBasedCFGTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedCFGTest, FallThroughSuccTest) {
  LLVMBasedCFG Cfg;
  ProjectIRDB IRDB({PathToLlFiles + "control_flow/branch_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING CONDITIONAL BRANCH
  // br i1 %5, label %6, label %9
  const auto *BranchInst = getNthTermInstruction(F, 1);
  // %7 = load i32, i32* %3, align 4
  ASSERT_FALSE(
      Cfg.isFallThroughSuccessor(BranchInst, getNthInstruction(F, 10)));
  // %10 = load i32, i32* %3, align 4
  ASSERT_TRUE(Cfg.isFallThroughSuccessor(BranchInst, getNthInstruction(F, 14)));

  // HANDLING UNCONDITIONAL BRANCH
  // br label %12
  BranchInst = getNthTermInstruction(F, 2);
  // ret i32 0
  ASSERT_TRUE(
      Cfg.isFallThroughSuccessor(BranchInst, getNthTermInstruction(F, 4)));
}

TEST_F(LLVMBasedCFGTest, BranchTargetTest) {
  LLVMBasedCFG Cfg;
  ProjectIRDB IRDB({PathToLlFiles + "control_flow/switch_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING SWITCH INSTRUCTION
  // switch i32 %4, label %8 [
  //   i32 65, label %5
  //   i32 66, label %6
  //   i32 67, label %6
  //   i32 68, label %7
  // ]
  const auto *SwitchInst = getNthTermInstruction(F, 1);
  // store i32 0, i32* %2, align 4
  ASSERT_FALSE(Cfg.isBranchTarget(SwitchInst, getNthStoreInstruction(F, 2)));
  // store i32 10, i32* %2, align 4
  ASSERT_TRUE(Cfg.isBranchTarget(SwitchInst, getNthStoreInstruction(F, 3)));
  // store i32 20, i32* %2, align 4
  ASSERT_TRUE(Cfg.isBranchTarget(SwitchInst, getNthStoreInstruction(F, 4)));
  // store i32 30, i32* %2, align 4
  ASSERT_TRUE(Cfg.isBranchTarget(SwitchInst, getNthStoreInstruction(F, 5)));
  // store i32 -1, i32* %2, align 4
  ASSERT_TRUE(Cfg.isBranchTarget(SwitchInst, getNthStoreInstruction(F, 6)));
  // ret i32 0
  ASSERT_FALSE(Cfg.isBranchTarget(SwitchInst, getNthTermInstruction(F, 6)));

  // HANDLING BRANCH INSTRUCTION
  // br label %9
  const auto *BranchInst = getNthTermInstruction(F, 2);
  // store i32 20, i32* %2, align 4
  ASSERT_FALSE(Cfg.isBranchTarget(BranchInst, getNthStoreInstruction(F, 4)));
  // ret i32 0
  ASSERT_TRUE(Cfg.isBranchTarget(BranchInst, getNthTermInstruction(F, 6)));
}

TEST_F(LLVMBasedCFGTest, HandlesMulitplePredeccessors) {
  LLVMBasedCFG Cfg;
  ProjectIRDB IRDB({PathToLlFiles + "control_flow/branch_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // ret i32 0
  const auto *TermInst = getNthTermInstruction(F, 4);
  std::vector<const llvm::Instruction *> Predeccessor;
  // br label %12
  Predeccessor.push_back(getNthTermInstruction(F, 2));
  // br label %12
  Predeccessor.push_back(getNthTermInstruction(F, 3));
  auto PredsOfTermInst = Cfg.getPredsOf(TermInst);
  ASSERT_EQ(PredsOfTermInst, Predeccessor);
}

TEST_F(LLVMBasedCFGTest, HandlesSingleOrEmptyPredeccessor) {
  LLVMBasedCFG Cfg;
  ProjectIRDB IRDB({PathToLlFiles + "control_flow/branch_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING SINGLE PREDECCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %3 = alloca i32, align 4)
  const auto *Pred = getNthInstruction(F, 3);
  std::vector<const llvm::Instruction *> Predeccessor{Pred};
  auto PredsOfInst = Cfg.getPredsOf(Inst);
  ASSERT_EQ(PredsOfInst, Predeccessor);

  // br i1 %11, label %12, label %16
  Inst = getNthTermInstruction(F, 1);
  // %5 = icmp sgt i32 1, %4
  Pred = getNthInstruction(F, 8);
  Predeccessor.clear();
  Predeccessor.push_back(Pred);
  PredsOfInst = Cfg.getPredsOf(Inst);
  ASSERT_EQ(PredsOfInst, Predeccessor);

  // HANDLING EMPTY PREDECCESSOR
  // %1 = alloca i32, align 4
  Inst = getNthInstruction(F, 1);
  PredsOfInst = Cfg.getPredsOf(Inst);
  Predeccessor.clear();
  ASSERT_EQ(PredsOfInst, Predeccessor);
}

TEST_F(LLVMBasedCFGTest, HandlesMultipleSuccessors) {
  LLVMBasedCFG Cfg;
  ProjectIRDB IRDB({PathToLlFiles + "control_flow/branch_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING CONDITIONAL BRANCH
  // br i1 %5, label %6, label %9
  const auto *BRInst = getNthTermInstruction(F, 1);
  std::vector<const llvm::Instruction *> Successors;
  // %7 = load i32, i32* %3, align 4
  Successors.push_back(getNthInstruction(F, 10));
  // %10 = load i32, i32* %3, align 4
  Successors.push_back(getNthInstruction(F, 14));
  auto SuccsOfBrInst = Cfg.getSuccsOf(BRInst);
  ASSERT_EQ(SuccsOfBrInst, Successors);

  // HANDLING UNCONDITIONAL BRANCH
  // br label %12
  BRInst = getNthTermInstruction(F, 3);
  Successors.clear();
  // ret i32 0
  Successors.push_back(getNthTermInstruction(F, 4));
  SuccsOfBrInst = Cfg.getSuccsOf(BRInst);
  ASSERT_EQ(SuccsOfBrInst, Successors);
}

TEST_F(LLVMBasedCFGTest, HandlesSingleOrEmptySuccessor) {
  LLVMBasedCFG Cfg;
  ProjectIRDB IRDB({PathToLlFiles + "control_flow/function_call_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING SINGLE SUCCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %4 = call i32 @_Z4multii(i32 2, i32 4)
  const auto *Succ = getNthInstruction(F, 5);
  std::vector<const llvm::Instruction *> Successors{Succ};
  auto SuccsOfInst = Cfg.getSuccsOf(Inst);
  ASSERT_EQ(SuccsOfInst, Successors);

  // HANDLING EMPTY SUCCESSOR
  // ret i32 0
  const auto *TermInst = getNthTermInstruction(F, 1);
  auto SuccsOfTermInst = Cfg.getSuccsOf(TermInst);
  Successors.clear();
  ASSERT_EQ(SuccsOfTermInst, Successors);
}

TEST_F(LLVMBasedCFGTest, HandlesCallSuccessor) {
  LLVMBasedCFG Cfg;
  ProjectIRDB IRDB({PathToLlFiles + "control_flow/function_call_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING CALL INSTRUCTION SUCCESSOR
  // %4 = call i32 @_Z4multii(i32 2, i32 4)
  const auto *CallInst = getNthInstruction(F, 5);
  // store i32 %4, i32* %2, align 4
  const auto *Succ = getNthStoreInstruction(F, 2);
  auto SuccsOfCallInst = Cfg.getSuccsOf(CallInst);
  std::vector<const llvm::Instruction *> Successors{Succ};
  ASSERT_EQ(SuccsOfCallInst, Successors);
}

TEST_F(LLVMBasedCFGTest, HandleFieldLoadsArray) {
  LLVMBasedCFG Cfg;
  ProjectIRDB IRDB({PathToLlFiles + "fields/array_1_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");
  const auto *Inst = getNthInstruction(F, 1);
  ASSERT_FALSE(Cfg.isFieldLoad(Inst));
  Inst = getNthInstruction(F, 6);
  ASSERT_TRUE(Cfg.isFieldLoad(Inst));
}

TEST_F(LLVMBasedCFGTest, HandleFieldStoreArray) {
  LLVMBasedCFG Cfg;
  ProjectIRDB IRDB({PathToLlFiles + "fields/array_1_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");
  const auto *Inst = getNthInstruction(F, 1);
  ASSERT_FALSE(Cfg.isFieldStore(Inst));
  Inst = getNthInstruction(F, 9);
  ASSERT_TRUE(Cfg.isFieldStore(Inst));
}

TEST_F(LLVMBasedCFGTest, HandleFieldLoadsField) {
  LLVMBasedCFG Cfg;
  ProjectIRDB IRDB({PathToLlFiles + "fields/field_1_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");
  const auto *Inst = getNthInstruction(F, 1);
  ASSERT_FALSE(Cfg.isFieldLoad(Inst));
  Inst = getNthInstruction(F, 11);
  ASSERT_TRUE(Cfg.isFieldLoad(Inst));
  Inst = getNthInstruction(F, 15);
  ASSERT_TRUE(Cfg.isFieldLoad(Inst));
  Inst = getNthInstruction(F, 19);
  ASSERT_TRUE(Cfg.isFieldLoad(Inst));
}

TEST_F(LLVMBasedCFGTest, HandleFieldStoreField) {
  LLVMBasedCFG Cfg;
  ProjectIRDB IRDB({PathToLlFiles + "fields/field_1_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");
  const auto *Inst = getNthInstruction(F, 1);
  ASSERT_FALSE(Cfg.isFieldStore(Inst));
  Inst = getNthInstruction(F, 5);
  ASSERT_TRUE(Cfg.isFieldStore(Inst));
  Inst = getNthInstruction(F, 7);
  ASSERT_TRUE(Cfg.isFieldStore(Inst));
  Inst = getNthInstruction(F, 9);
  ASSERT_TRUE(Cfg.isFieldStore(Inst));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
