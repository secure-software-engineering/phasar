#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/Utils/Utilities.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

class LLVMGetterTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

TEST_F(LLVMGetterTest, HandlesLLVMStoreInstruction) {
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/global_stmt_cpp.ll"});
  auto F = IRDB.getFunctionDefinition("main");
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
  auto F = IRDB.getFunctionDefinition("main");
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

  auto F =
      IRDB.getModule(pathToLLFiles + "name_mangling/special_members_2_cpp.ll");
  auto &m = *F->getFunction("_ZNSt8ios_base4InitC1Ev");
  ASSERT_EQ(specialMemberFunctionType(m.getName()),
            SpecialMemberFunctionTy::CTOR);
  auto &n = *F->getFunction("_ZNSt8ios_base4InitD1Ev");
  ASSERT_EQ(specialMemberFunctionType(n.getName()),
            SpecialMemberFunctionTy::DTOR);
  auto &o = *F->getFunction(
      "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev");
  ASSERT_EQ(specialMemberFunctionType(o.getName()),
            SpecialMemberFunctionTy::DTOR);
}

TEST_F(LLVMGetterTest, HandlesCppUserDefinedType) {
  ProjectIRDB IRDB({pathToLLFiles + "name_mangling/special_members_1_cpp.ll"});

  auto F =
      IRDB.getModule(pathToLLFiles + "name_mangling/special_members_1_cpp.ll");
  auto &m = *F->getFunction("_ZN7MyClassC2Ev");
  ASSERT_EQ(specialMemberFunctionType(m.getName()),
            SpecialMemberFunctionTy::CTOR);
  auto &n = *F->getFunction("_ZN7MyClassaSERKS_");
  ASSERT_EQ(specialMemberFunctionType(n.getName()),
            SpecialMemberFunctionTy::CPASSIGNOP);
  auto &o = *F->getFunction("_ZN7MyClassaSEOS_");
  ASSERT_EQ(specialMemberFunctionType(o.getName()),
            SpecialMemberFunctionTy::MVASSIGNOP);
}

TEST_F(LLVMGetterTest, HandlesCppNonStandardFunctions) {
  ProjectIRDB IRDB({pathToLLFiles + "name_mangling/special_members_3_cpp.ll"});

  auto F =
      IRDB.getModule(pathToLLFiles + "name_mangling/special_members_3_cpp.ll");
  auto &m = *F->getFunction("_ZN9testspace3foo3barES0_");
  ASSERT_EQ(specialMemberFunctionType(m.getName()),
            SpecialMemberFunctionTy::NONE);
}

TEST_F(LLVMGetterTest, HandleFunctionsContainingCodesInName) {
  ProjectIRDB IRDB({pathToLLFiles + "name_mangling/special_members_4_cpp.ll"});

  auto M =
      IRDB.getModule(pathToLLFiles + "name_mangling/special_members_4_cpp.ll");
  auto F = M->getFunction("_ZN8C0C1C2C12D1C2Ev"); // C0C1C2C1::D1::D1()
  std::cout << "VALUE IS: "
            << static_cast<std::underlying_type_t<SpecialMemberFunctionTy>>(
                   specialMemberFunctionType(F->getName()))
            << std::endl;
  ASSERT_EQ(specialMemberFunctionType(F->getName()),
            SpecialMemberFunctionTy::CTOR);
  F = M->getFunction(
      "_ZN8C0C1C2C12D1C2ERKS0_"); // C0C1C2C1::D1::D1(C0C1C2C1::D1 const&)
  ASSERT_EQ(specialMemberFunctionType(F->getName()),
            SpecialMemberFunctionTy::CTOR);
  F = M->getFunction(
      "_ZN8C0C1C2C12D1C2EOS0_"); // C0C1C2C1::D1::D1(C0C1C2C1::D1&&)
  ASSERT_EQ(specialMemberFunctionType(F->getName()),
            SpecialMemberFunctionTy::CTOR);
  F = M->getFunction("_ZN8C0C1C2C12D1D2Ev"); // C0C1C2C1::D1::~D1()
  ASSERT_EQ(specialMemberFunctionType(F->getName()),
            SpecialMemberFunctionTy::DTOR);
  F = M->getFunction("_Z12C1C2C3D0D1D2v"); // C1C2C3D0D1D2()
  ASSERT_EQ(specialMemberFunctionType(F->getName()),
            SpecialMemberFunctionTy::NONE);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
