#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Macros.h>

using namespace std;
using namespace psr;

class LLVMGetterTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/";
};

TEST_F(LLVMGetterTest, HandlesLLVMStoreInstruction) {
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/global_stmt_cpp.ll"});
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

TEST_F(LLVMGetterTest, HandlesLLVMTermInstruction) {
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/if_else_cpp.ll"});
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
