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
  const std::string PathToLlFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

TEST_F(LLVMGetterTest, HandlesLLVMStoreInstruction) {
  ProjectIRDB IRDB({PathToLlFiles + "control_flow/global_stmt_cpp.ll"});
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

TEST_F(LLVMGetterTest, HandlesLLVMTermInstruction) {
  ProjectIRDB IRDB({PathToLlFiles + "control_flow/if_else_cpp.ll"});
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

TEST_F(LLVMGetterTest, HandlesCppStandardType) {
  ProjectIRDB IRDB({PathToLlFiles + "name_mangling/special_members_2_cpp.ll"});

  auto *F =
      IRDB.getModule(PathToLlFiles + "name_mangling/special_members_2_cpp.ll");
  auto &M = *F->getFunction("_ZNSt8ios_base4InitC1Ev");
  ASSERT_EQ(specialMemberFunctionType(M.getName()),
            SpecialMemberFunctionTy::CTOR);
  auto &N = *F->getFunction("_ZNSt8ios_base4InitD1Ev");
  ASSERT_EQ(specialMemberFunctionType(N.getName()),
            SpecialMemberFunctionTy::DTOR);
  auto &O = *F->getFunction(
      "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev");
  ASSERT_EQ(specialMemberFunctionType(O.getName()),
            SpecialMemberFunctionTy::DTOR);
}

TEST_F(LLVMGetterTest, HandlesCppUserDefinedType) {
  ProjectIRDB IRDB({PathToLlFiles + "name_mangling/special_members_1_cpp.ll"});

  auto *F =
      IRDB.getModule(PathToLlFiles + "name_mangling/special_members_1_cpp.ll");
  auto &M = *F->getFunction("_ZN7MyClassC2Ev");
  ASSERT_EQ(specialMemberFunctionType(M.getName()),
            SpecialMemberFunctionTy::CTOR);
  auto &N = *F->getFunction("_ZN7MyClassaSERKS_");
  ASSERT_EQ(specialMemberFunctionType(N.getName()),
            SpecialMemberFunctionTy::CPASSIGNOP);
  auto &O = *F->getFunction("_ZN7MyClassaSEOS_");
  ASSERT_EQ(specialMemberFunctionType(O.getName()),
            SpecialMemberFunctionTy::MVASSIGNOP);
}

TEST_F(LLVMGetterTest, HandlesCppNonStandardFunctions) {
  ProjectIRDB IRDB({PathToLlFiles + "name_mangling/special_members_3_cpp.ll"});

  auto *F =
      IRDB.getModule(PathToLlFiles + "name_mangling/special_members_3_cpp.ll");
  auto &M = *F->getFunction("_ZN9testspace3foo3barES0_");
  ASSERT_EQ(specialMemberFunctionType(M.getName()),
            SpecialMemberFunctionTy::NONE);
}

TEST_F(LLVMGetterTest, HandleFunctionsContainingCodesInName) {
  ProjectIRDB IRDB({PathToLlFiles + "name_mangling/special_members_4_cpp.ll"});

  auto *M =
      IRDB.getModule(PathToLlFiles + "name_mangling/special_members_4_cpp.ll");
  auto *F = M->getFunction("_ZN8C0C1C2C12D1C2Ev"); // C0C1C2C1::D1::D1()
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

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
