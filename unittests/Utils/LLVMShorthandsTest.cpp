#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>

using namespace psr;

TEST(LLVMGetterTest, HandlesLLVMStoreInstruction) {
  ProjectIRDB IRDB(
      {"../../../test/llvm_test_code/control_flow/global_stmt.ll"});
  auto F = IRDB.getFunction("main");
  ASSERT_EQ(getNthStoreInstruction(F, 0), nullptr);
  auto I = getNthInstruction(F, 4);
  ASSERT_EQ(getNthStoreInstruction(F, 1), I);
  I = getNthInstruction(F, 5);
  ASSERT_EQ(getNthStoreInstruction(F, 2), I);
  I = getNthInstruction(F, 7);
  ASSERT_EQ(getNthStoreInstruction(F, 3), I);
  ASSERT_EQ(getNthStoreInstruction(F, 4), nullptr);
}

TEST(LLVMGetterTest, HandlesLLVMTermInstruction) {
  ProjectIRDB IRDB({"../../../test/llvm_test_code/control_flow/if_else.ll"});
  auto F = IRDB.getFunction("main");
  ASSERT_EQ(getNthTermInstruction(F, 0), nullptr);
  auto I = getNthInstruction(F, 14);
  ASSERT_EQ(getNthTermInstruction(F, 1), I);
  I = getNthInstruction(F, 21);
  ASSERT_EQ(getNthTermInstruction(F, 2), I);
  I = getNthInstruction(F, 25);
  ASSERT_EQ(getNthTermInstruction(F, 3), I);
  I = getNthInstruction(F, 27);
  ASSERT_EQ(getNthTermInstruction(F, 4), I);
  ASSERT_EQ(getNthTermInstruction(F, 5), nullptr);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}