#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

TEST(LLVMGetterTest, HandlesLLVMStoreInstruction) {
  LLVMProjectIRDB IRDB({"llvm_test_code/control_flow/global_stmt.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");
  ASSERT_EQ(getNthStoreInstruction(F, 0), nullptr);
  const auto *I = getNthInstruction(F, 4);
  ASSERT_EQ(getNthStoreInstruction(F, 1), I);
  I = getNthInstruction(F, 5);
  ASSERT_EQ(getNthStoreInstruction(F, 2), I);
  I = getNthInstruction(F, 7);
  ASSERT_EQ(getNthStoreInstruction(F, 3), I);
  ASSERT_EQ(getNthStoreInstruction(F, 4), nullptr);
}

TEST(LLVMGetterTest, HandlesLLVMTermInstruction) {
  LLVMProjectIRDB IRDB({"llvm_test_code/control_flow/if_else.ll"});
  const auto *F = IRDB.getFunctionDefinition("main");
  ASSERT_EQ(getNthTermInstruction(F, 0), nullptr);
  const auto *I = getNthInstruction(F, 14);
  ASSERT_EQ(getNthTermInstruction(F, 1), I);
  I = getNthInstruction(F, 21);
  ASSERT_EQ(getNthTermInstruction(F, 2), I);
  I = getNthInstruction(F, 25);
  ASSERT_EQ(getNthTermInstruction(F, 3), I);
  I = getNthInstruction(F, 27);
  ASSERT_EQ(getNthTermInstruction(F, 4), I);
  ASSERT_EQ(getNthTermInstruction(F, 5), nullptr);
}

TEST(SlotTrackerTest, HandleTwoReferences) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "control_flow/global_stmt.ll");

  const auto *F = IRDB.getFunctionDefinition("main");

  ASSERT_NE(F, nullptr);
  const auto *Inst = getNthInstruction(F, 6);
  llvm::StringRef InstStr = "%0 = load i32, i32* @i, align 4 | ID: 6";
  {
    LLVMProjectIRDB IRDB2(IRDB.getModule());

    EXPECT_EQ(llvmIRToStableString(Inst), InstStr);
  }

  EXPECT_EQ(llvmIRToStableString(Inst), InstStr);
}
