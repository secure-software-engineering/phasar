#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"

#include "phasar/Config/Configuration.h"
#include "phasar/ControlFlow/SpecialMemberFunctionType.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

TEST(LLVMBasedCFGTest, FallThroughSuccTest) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "control_flow/branch_cpp.ll");
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

TEST(LLVMBasedCFGTest, BranchTargetTest) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "control_flow/switch_cpp.ll");
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

TEST(LLVMBasedCFGTest, HandlesMulitplePredeccessors) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "control_flow/branch_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // ret i32 0
  const auto *TermInst = getNthTermInstruction(F, 4);
  llvm::SmallVector<const llvm::Instruction *> Predeccessor;
  // br label %12
  Predeccessor.push_back(getNthTermInstruction(F, 3));
  // br label %12
  Predeccessor.push_back(getNthTermInstruction(F, 2));
  auto PredsOfTermInst = Cfg.getPredsOf(TermInst);
  ASSERT_EQ(PredsOfTermInst, Predeccessor);
}

TEST(LLVMBasedCFGTest, HandlesSingleOrEmptyPredeccessor) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "control_flow/branch_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING SINGLE PREDECCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %3 = alloca i32, align 4)
  const auto *Pred = getNthInstruction(F, 3);
  llvm::SmallVector<const llvm::Instruction *> Predeccessor{Pred};
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

TEST(LLVMBasedCFGTest, HandlesMultipleSuccessors) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "control_flow/branch_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING CONDITIONAL BRANCH
  // br i1 %5, label %6, label %9
  const auto *BRInst = getNthTermInstruction(F, 1);
  llvm::SmallVector<const llvm::Instruction *> Successors;
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

TEST(LLVMBasedCFGTest, HandlesSingleOrEmptySuccessor) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "control_flow/function_call_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING SINGLE SUCCESSOR
  // store i32 0, i32* %1, align 4
  const llvm::Instruction *Inst = getNthStoreInstruction(F, 1);
  // %4 = call i32 @_Z4multii(i32 2, i32 4)
  const auto *Succ = getNthInstruction(F, 5);
  llvm::SmallVector<const llvm::Instruction *> Successors{Succ};
  auto SuccsOfInst = Cfg.getSuccsOf(Inst);
  ASSERT_EQ(SuccsOfInst, Successors);

  // HANDLING EMPTY SUCCESSOR
  // ret i32 0
  const auto *TermInst = getNthTermInstruction(F, 1);
  auto SuccsOfTermInst = Cfg.getSuccsOf(TermInst);
  Successors.clear();
  ASSERT_EQ(SuccsOfTermInst, Successors);
}

TEST(LLVMBasedCFGTest, HandlesCallSuccessor) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "control_flow/function_call_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");

  // HANDLING CALL INSTRUCTION SUCCESSOR
  // %4 = call i32 @_Z4multii(i32 2, i32 4)
  const auto *CallInst = getNthInstruction(F, 5);
  // store i32 %4, i32* %2, align 4
  const auto *Succ = getNthStoreInstruction(F, 2);
  auto SuccsOfCallInst = Cfg.getSuccsOf(CallInst);
  llvm::SmallVector<const llvm::Instruction *> Successors{Succ};
  ASSERT_EQ(SuccsOfCallInst, Successors);
}

TEST(LLVMBasedCFGTest, HandleFieldLoadsArray) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles + "fields/array_1_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");
  const auto *Inst = getNthInstruction(F, 1);
  ASSERT_FALSE(Cfg.isFieldLoad(Inst));
  Inst = getNthInstruction(F, 6);
  ASSERT_TRUE(Cfg.isFieldLoad(Inst));
}

TEST(LLVMBasedCFGTest, HandleFieldStoreArray) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles + "fields/array_1_cpp.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");
  const auto *Inst = getNthInstruction(F, 1);
  ASSERT_FALSE(Cfg.isFieldStore(Inst));
  Inst = getNthInstruction(F, 9);
  ASSERT_TRUE(Cfg.isFieldStore(Inst));
}

TEST(LLVMBasedCFGTest, HandleFieldLoadsField) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles + "fields/field_1_cpp.ll"});
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

TEST(LLVMBasedCFGTest, HandleFieldStoreField) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles + "fields/field_1_cpp.ll"});
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

PHASAR_SKIP_TEST(TEST(LLVMBasedCFGTest, HandlesCppStandardType) {
  // If we use libcxx this won't work since internal implementation is different
  LIBCPP_GTEST_SKIP;

  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "name_mangling/special_members_2_cpp.ll"});

  auto *M = IRDB.getModule();
  auto *F = M->getFunction("_ZNSt8ios_base4InitC1Ev");
  LLVMBasedCFG CFG;
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(F),
            SpecialMemberFunctionType::Constructor);
  auto *N = M->getFunction("_ZNSt8ios_base4InitD1Ev");
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(N),
            SpecialMemberFunctionType::Destructor);
  auto *O = M->getFunction(
      "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev");
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(O),
            SpecialMemberFunctionType::Destructor);
})

TEST(LLVMBasedCFGTest, HandlesCppUserDefinedType) {
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "name_mangling/special_members_1_cpp.ll"});

  auto *M = IRDB.getModule();
  auto *F = M->getFunction("_ZN7MyClassC2Ev");
  LLVMBasedCFG CFG;
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(F),
            SpecialMemberFunctionType::Constructor);
  auto *N = M->getFunction("_ZN7MyClassaSERKS_");
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(N),
            SpecialMemberFunctionType::CopyAssignment);
  auto *O = M->getFunction("_ZN7MyClassaSEOS_");
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(O),
            SpecialMemberFunctionType::MoveAssignment);
}

TEST(LLVMBasedCFGTest, HandlesCppNonStandardFunctions) {
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "name_mangling/special_members_3_cpp.ll"});

  auto *M = IRDB.getModule();
  auto *F = M->getFunction("_ZN9testspace3foo3barES0_");
  LLVMBasedCFG CFG;
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(F),
            SpecialMemberFunctionType::None);
}

TEST(LLVMBasedCFGTest, HandleFunctionsContainingCodesInName) {
  LLVMProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "name_mangling/special_members_4_cpp.ll"});

  auto *M = IRDB.getModule();
  auto *F = M->getFunction("_ZN8C0C1C2C12D1C2Ev"); // C0C1C2C1::D1::D1()
  LLVMBasedCFG CFG;
  std::cout << "VALUE IS: "
            << static_cast<std::underlying_type_t<SpecialMemberFunctionType>>(
                   CFG.getSpecialMemberFunctionType(F))
            << std::endl;
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(F),
            SpecialMemberFunctionType::Constructor);
  F = M->getFunction(
      "_ZN8C0C1C2C12D1C2ERKS0_"); // C0C1C2C1::D1::D1(C0C1C2C1::D1 const&)
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(F),
            SpecialMemberFunctionType::Constructor);
  F = M->getFunction(
      "_ZN8C0C1C2C12D1C2EOS0_"); // C0C1C2C1::D1::D1(C0C1C2C1::D1&&)
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(F),
            SpecialMemberFunctionType::Constructor);
  F = M->getFunction("_ZN8C0C1C2C12D1D2Ev"); // C0C1C2C1::D1::~D1()
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(F),
            SpecialMemberFunctionType::Destructor);
  F = M->getFunction("_Z12C1C2C3D0D1D2v"); // C1C2C3D0D1D2()
  ASSERT_EQ(CFG.getSpecialMemberFunctionType(F),
            SpecialMemberFunctionType::None);
}

TEST(LLVMBasedCFGTest, IgnoreSingleDbgInstructionsInSuccessors) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB1({unittest::PathToLLTestFiles +
                         "control_flow/ignore_dbg_insts_1_cpp_dbg.ll"});
  const auto *F = IRDB1.getFunctionDefinition("main");
  const auto *I1 = getNthInstruction(F, 4);
  // Ask a non-debug instructions for its successors
  auto Succs1 = Cfg.getSuccsOf(I1);
  const auto *I2 = getNthInstruction(F, 6);
  ASSERT_EQ(Succs1.size(), 1U);
  ASSERT_EQ(Succs1[0], I2);
  // Ask debug instruction for its sucessors
  const auto *I3 = getNthInstruction(F, 7);
  auto Succs2 = Cfg.getSuccsOf(I3);
  const auto *I4 = getNthInstruction(F, 8);
  ASSERT_EQ(Succs2.size(), 1U);
  ASSERT_EQ(Succs2[0], I4);
}

TEST(LLVMBasedCFGTest, IgnoreMultiSubsequentDbgInstructionsInSuccessors) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB1({unittest::PathToLLTestFiles +
                         "control_flow/ignore_dbg_insts_4_cpp_dbg.ll"});
  const auto *F = IRDB1.getFunctionDefinition("main");
  const auto *I1 = getNthInstruction(F, 5);
  // Ask a non-debug instructions for its successors
  auto Succs1 = Cfg.getSuccsOf(I1);
  const auto *I2 = getNthInstruction(F, 9);
  ASSERT_EQ(Succs1.size(), 1U);
  ASSERT_EQ(Succs1[0], I2);
  // Ask debug instruction for its sucessors
  const auto *I3 = getNthInstruction(F, 6);
  auto Succs2 = Cfg.getSuccsOf(I3);
  const auto *I4 = getNthInstruction(F, 9);
  ASSERT_EQ(Succs2.size(), 1U);
  ASSERT_EQ(Succs2[0], I4);
}

TEST(LLVMBasedCFGTest, IgnoreSingleDbgInstructionsInPredecessors) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB1({unittest::PathToLLTestFiles +
                         "control_flow/ignore_dbg_insts_1_cpp_dbg.ll"});
  const auto *F = IRDB1.getFunctionDefinition("main");
  const auto *I1 = getNthInstruction(F, 6);
  // Ask a non-debug instructions for its successors
  auto Preds1 = Cfg.getPredsOf(I1);
  const auto *I2 = getNthInstruction(F, 4);
  ASSERT_EQ(Preds1.size(), 1U);
  ASSERT_EQ(Preds1[0], I2);
  // Ask debug instruction for its sucessors
  const auto *I3 = getNthInstruction(F, 5);
  auto Preds2 = Cfg.getPredsOf(I3);
  const auto *I4 = getNthInstruction(F, 4);
  ASSERT_EQ(Preds2.size(), 1U);
  ASSERT_EQ(Preds2[0], I4);
}

TEST(LLVMBasedCFGTest, IgnoreMultiSubsequentDbgInstructionsInPredecessors) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB1({unittest::PathToLLTestFiles +
                         "control_flow/ignore_dbg_insts_4_cpp_dbg.ll"});
  const auto *F = IRDB1.getFunctionDefinition("main");
  const auto *I1 = getNthInstruction(F, 9);
  // Ask a non-debug instructions for its successors
  auto Preds1 = Cfg.getPredsOf(I1);
  const auto *I2 = getNthInstruction(F, 5);
  ASSERT_EQ(Preds1.size(), 1U);
  ASSERT_EQ(Preds1[0], I2);
  // Ask debug instruction for its sucessors
  const auto *I3 = getNthInstruction(F, 7);
  auto Preds2 = Cfg.getPredsOf(I3);
  const auto *I4 = getNthInstruction(F, 5);
  ASSERT_EQ(Preds2.size(), 1U);
  ASSERT_EQ(Preds2[0], I4);
}

TEST(LLVMBasedCFGTest, IgnoreSingleDbgInstructionsInControlFlowEdges) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB1({unittest::PathToLLTestFiles +
                         "control_flow/ignore_dbg_insts_1_cpp_dbg.ll"});
  const auto *F = IRDB1.getFunctionDefinition("main");
  auto ControlFlowEdges = Cfg.getAllControlFlowEdges(F);
  for (const auto &[Src, Dst] : ControlFlowEdges) {
    // Calls to the intrinsic debug functions are disallowed here
    if (const auto *CallInst = llvm::dyn_cast<llvm::CallInst>(Src)) {
      ASSERT_FALSE(CallInst->getCalledFunction() &&
                   CallInst->getCalledFunction()->isIntrinsic());
    }
    if (const auto *CallInst = llvm::dyn_cast<llvm::CallInst>(Dst)) {
      ASSERT_FALSE(CallInst->getCalledFunction() &&
                   CallInst->getCalledFunction()->isIntrinsic());
    }
  }
}

TEST(LLVMBasedCFGTest, IgnoreMultiSubsequentDbgInstructionsInControlFlowEdges) {
  LLVMBasedCFG Cfg;
  LLVMProjectIRDB IRDB1({unittest::PathToLLTestFiles +
                         "control_flow/ignore_dbg_insts_4_cpp_dbg.ll"});
  const auto *F = IRDB1.getFunctionDefinition("main");
  auto ControlFlowEdges = Cfg.getAllControlFlowEdges(F);
  for (const auto &[Src, Dst] : ControlFlowEdges) {
    // Calls to the intrinsic debug functions are disallowed here
    if (const auto *CallInst = llvm::dyn_cast<llvm::CallInst>(Src)) {
      ASSERT_FALSE(CallInst->getCalledFunction() &&
                   CallInst->getCalledFunction()->isIntrinsic());
    }
    if (const auto *CallInst = llvm::dyn_cast<llvm::CallInst>(Dst)) {
      ASSERT_FALSE(CallInst->getCalledFunction() &&
                   CallInst->getCalledFunction()->isIntrinsic());
    }
  }
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
