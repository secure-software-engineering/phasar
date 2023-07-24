#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

TEST(LLVMBasedBackwardCFGTest, BranchTargetTest) {
  LLVMBasedBackwardCFG Cfg;
  LLVMProjectIRDB IRDB({"llvm_test_code/control_flow/branch.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");
  const auto *Term = getNthTermInstruction(F, 1);
  const auto *A = getNthInstruction(F, 10);
  const auto *B = getNthInstruction(F, 14);
  const auto *C = getNthInstruction(F, 12);

  ASSERT_TRUE(Cfg.isBranchTarget(A, Term));
  ASSERT_TRUE(Cfg.isBranchTarget(B, Term));
  ASSERT_FALSE(Cfg.isBranchTarget(C, Term));
}

TEST(LLVMBasedBackwardCFGTest, HandlesMulitplePredeccessors) {
  LLVMBasedBackwardCFG Cfg;
  LLVMProjectIRDB IRDB({"llvm_test_code/control_flow/branch.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING CONDITIONAL BRANCH
  // br i1 %5, label %6, label %9
  const auto *BRInst = getNthTermInstruction(F, 1);
  llvm::SmallVector<const llvm::Instruction *> Predeccessors;
  // %7 = load i32, i32* %3, align 4
  Predeccessors.push_back(getNthInstruction(F, 10));
  // %10 = load i32, i32* %3, align 4
  Predeccessors.push_back(getNthInstruction(F, 14));
  auto PredsOfBrInst = Cfg.getPredsOf(BRInst);
  ASSERT_EQ(PredsOfBrInst, Predeccessors);

  // HANDLING UNCONDITIONAL BRANCH
  // br label %12
  BRInst = getNthTermInstruction(F, 3);
  Predeccessors.clear();
  // ret i32 0
  Predeccessors.push_back(getNthTermInstruction(F, 4));
  PredsOfBrInst = Cfg.getPredsOf(BRInst);
  ASSERT_EQ(PredsOfBrInst, Predeccessors);
}

TEST(LLVMBasedBackwardCFGTest, HandlesSingleOrEmptyPredeccessor) {
  LLVMBasedBackwardCFG Cfg;
  LLVMProjectIRDB IRDB({"llvm_test_code/control_flow/function_call.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING SINGLE PREDECCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %4 = call i32 @_Z4multii(i32 2, i32 4)
  const auto *Pred = getNthInstruction(F, 5);
  llvm::SmallVector<const llvm::Instruction *> Predeccessor{Pred};
  auto PredsOfInst = Cfg.getPredsOf(Inst);
  ASSERT_EQ(PredsOfInst, Predeccessor);

  // HANDLING EMPTY SUCCESSOR
  // ret i32 0
  const auto *TermInst = getNthTermInstruction(F, 1);
  auto PredsOfTermInst = Cfg.getPredsOf(TermInst);
  Predeccessor.clear();
  ASSERT_EQ(PredsOfTermInst, Predeccessor);
}

TEST(LLVMBasedBackwardCFGTest, HandlesMultipleSuccessors) {
  LLVMBasedBackwardCFG Cfg;
  LLVMProjectIRDB IRDB({"llvm_test_code/control_flow/branch.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // ret i32 0
  const auto *TermInst = getNthTermInstruction(F, 4);
  std::cout << llvmIRToString(TermInst) << std::endl;
  llvm::SmallVector<const llvm::Instruction *> Successor;
  // br label %12
  Successor.push_back(getNthTermInstruction(F, 3));
  // br label %12
  Successor.push_back(getNthTermInstruction(F, 2));
  auto SuccsOfTermInst = Cfg.getSuccsOf(TermInst);
  ASSERT_EQ(SuccsOfTermInst, Successor);
}

TEST(LLVMBasedBackwardCFGTest, HandlesSingleOrEmptySuccessor) {
  LLVMBasedBackwardCFG Cfg;
  LLVMProjectIRDB IRDB({"llvm_test_code/control_flow/branch.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING SINGLE SUCCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %3 = alloca i32, align 4)
  const auto *Succ = getNthInstruction(F, 3);
  llvm::SmallVector<const llvm::Instruction *> Successor{Succ};
  auto SuccsOfInst = Cfg.getSuccsOf(Inst);
  ASSERT_EQ(SuccsOfInst, Successor);

  // br i1 %11, label %12, label %16
  Inst = getNthTermInstruction(F, 1);
  // %5 = icmp sgt i32 1, %4
  Succ = getNthInstruction(F, 8);
  Successor.clear();
  Successor.push_back(Succ);
  SuccsOfInst = Cfg.getSuccsOf(Inst);
  ASSERT_EQ(SuccsOfInst, Successor);

  // HANDLING EMPTY SUCCESSOR
  // %1 = alloca i32, align 4
  Inst = getNthInstruction(F, 1);
  SuccsOfInst = Cfg.getSuccsOf(Inst);
  Successor.clear();
  ASSERT_EQ(SuccsOfInst, Successor);
}
