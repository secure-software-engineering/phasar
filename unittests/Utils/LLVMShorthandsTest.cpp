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

TEST_F(LLVMGetterTest, HandlesCppStandardType) {
  ProjectIRDB IRDB({pathToLLFiles + "name_mangling/special_members_2_cpp.ll"});
  
  auto F = IRDB.getModule(pathToLLFiles+"name_mangling/special_members_2_cpp.ll");
  auto &m = *F->getFunction("_ZNSt8ios_base4InitC1Ev");
  ASSERT_EQ(specialMemberFunctionType(m.getName()),FuncType::ctor);
  auto &n = *F->getFunction("_ZNSt8ios_base4InitD1Ev");
  ASSERT_EQ(specialMemberFunctionType(n.getName()),FuncType::dtor);
  auto &o = *F->getFunction("_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev");
  ASSERT_EQ(specialMemberFunctionType(o.getName()),FuncType::dtor);
}

TEST_F(LLVMGetterTest, HandlesCppUserDefinedType) {
  ProjectIRDB IRDB({pathToLLFiles + "name_mangling/special_members_1_cpp.ll"});
  
  auto F = IRDB.getModule(pathToLLFiles+"name_mangling/special_members_1_cpp.ll");
  auto &m = *F->getFunction("_ZN7MyClassC2Ev");
  ASSERT_EQ(specialMemberFunctionType(m.getName()),FuncType::ctor);
  auto &n = *F->getFunction("_ZN7MyClassaSERKS_");
  ASSERT_EQ(specialMemberFunctionType(n.getName()),FuncType::cpyasmtopr);
  auto &o = *F->getFunction("_ZN7MyClassaSEOS_");
  ASSERT_EQ(specialMemberFunctionType(o.getName()),FuncType::movasmtopr);
}

TEST_F(LLVMGetterTest, HandlesCppNonStandardFunctions) {
  ProjectIRDB IRDB({pathToLLFiles + "name_mangling/special_members_3_cpp.ll"});
  
  auto F = IRDB.getModule(pathToLLFiles+"name_mangling/special_members_3_cpp.ll");
  auto &m = *F->getFunction("_ZN9testspace3foo3barES0_");
  ASSERT_EQ(specialMemberFunctionType(m.getName()),FuncType::none);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
