#include "gtest/gtest.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

TEST(LLVMGetterTest, HandlesLLVMStoreInstruction) {
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "control_flow/global_stmt_cpp.ll"});
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
  ProjectIRDB IRDB(
      {unittest::PathToLLTestFiles + "control_flow/if_else_cpp.ll"});
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

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
