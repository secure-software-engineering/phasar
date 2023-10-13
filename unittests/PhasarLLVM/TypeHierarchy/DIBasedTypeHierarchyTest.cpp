
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <iomanip>

namespace psr {

/*
---------------------------
BasicTHReconstruction Tests
---------------------------
*/

TEST(DBTHTest, BasicTHReconstruction_1) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_1_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_2) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_2_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_3) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_3_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_4) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_4_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_5) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_5_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 3U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &OtherBaseType = DBTH.getType("OtherBase");
  ASSERT_NE(nullptr, OtherBaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(OtherBaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypesBase = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypesBase.find(ChildType) != SubTypesBase.end());
  const auto &SubTypesOtherBase = DBTH.getSubTypes(OtherBaseType);
  EXPECT_TRUE(SubTypesOtherBase.find(ChildType) != SubTypesOtherBase.end());
}

TEST(DBTHTest, BasicTHReconstruction_6) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_6_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_7) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_7_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 7U);
  const auto &AType = DBTH.getType("A");
  ASSERT_NE(nullptr, AType);
  const auto &BType = DBTH.getType("B");
  ASSERT_NE(nullptr, BType);
  const auto &CType = DBTH.getType("C");
  ASSERT_NE(nullptr, CType);
  const auto &DType = DBTH.getType("D");
  ASSERT_NE(nullptr, DType);
  const auto &XType = DBTH.getType("X");
  ASSERT_NE(nullptr, XType);
  const auto &YType = DBTH.getType("Y");
  ASSERT_NE(nullptr, YType);
  const auto &ZType = DBTH.getType("Z");
  ASSERT_NE(nullptr, ZType);

  EXPECT_TRUE(DBTH.hasType(AType));
  EXPECT_TRUE(DBTH.hasType(BType));
  EXPECT_TRUE(DBTH.hasType(CType));
  EXPECT_TRUE(DBTH.hasType(DType));
  EXPECT_TRUE(DBTH.hasType(XType));
  EXPECT_TRUE(DBTH.hasType(YType));
  EXPECT_TRUE(DBTH.hasType(ZType));

  // check for all subtypes

  // struct B : A {};
  // struct C : A {};
  const auto &SubTypesA = DBTH.getSubTypes(AType);
  EXPECT_TRUE(SubTypesA.find(BType) != SubTypesA.end());
  EXPECT_TRUE(SubTypesA.find(CType) != SubTypesA.end());
  // struct D : B {};
  const auto &SubTypesB = DBTH.getSubTypes(BType);
  EXPECT_TRUE(SubTypesB.find(DType) != SubTypesB.end());
  // struct Z : C, Y {};
  const auto &SubTypesC = DBTH.getSubTypes(CType);
  EXPECT_TRUE(SubTypesC.find(ZType) != SubTypesC.end());
  // struct Y : X {};
  const auto &SubTypesX = DBTH.getSubTypes(XType);
  EXPECT_TRUE(SubTypesX.find(YType) != SubTypesX.end());
  // struct Z : C, Y {};
  const auto &SubTypesY = DBTH.getSubTypes(YType);
  EXPECT_TRUE(SubTypesY.find(ZType) != SubTypesY.end());
}

TEST(DBTHTest, BasicTHReconstruction_7_b) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_7_b_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 7U);
  const auto &AType = DBTH.getType("A");
  ASSERT_NE(nullptr, AType);
  const auto &CType = DBTH.getType("C");
  ASSERT_NE(nullptr, CType);
  const auto &XType = DBTH.getType("X");
  ASSERT_NE(nullptr, XType);
  const auto &YType = DBTH.getType("Y");
  ASSERT_NE(nullptr, YType);
  const auto &ZType = DBTH.getType("Z");
  ASSERT_NE(nullptr, ZType);
  const auto &OmegaType = DBTH.getType("Omega");
  ASSERT_NE(nullptr, OmegaType);

  EXPECT_TRUE(DBTH.hasType(AType));
  EXPECT_TRUE(DBTH.hasType(CType));
  EXPECT_TRUE(DBTH.hasType(XType));
  EXPECT_TRUE(DBTH.hasType(YType));
  EXPECT_TRUE(DBTH.hasType(ZType));
  EXPECT_TRUE(DBTH.hasType(OmegaType));

  // check for all subtypes

  // struct C : A {};
  const auto &SubTypesA = DBTH.getSubTypes(AType);
  EXPECT_TRUE(SubTypesA.find(CType) != SubTypesA.end());
  // struct Z : C, Y {};
  const auto &SubTypesC = DBTH.getSubTypes(CType);
  EXPECT_TRUE(SubTypesC.find(ZType) != SubTypesC.end());
  // struct Y : X {};
  const auto &SubTypesX = DBTH.getSubTypes(XType);
  EXPECT_TRUE(SubTypesX.find(YType) != SubTypesX.end());
  // struct Z : C, Y {};
  const auto &SubTypesY = DBTH.getSubTypes(YType);
  EXPECT_TRUE(SubTypesY.find(ZType) != SubTypesY.end());

  // class Omega : Z {
  const auto &SubTypesZ = DBTH.getSubTypes(ZType);
  EXPECT_TRUE(SubTypesZ.find(OmegaType) != SubTypesZ.end());
}

TEST(DBTHTest, BasicTHReconstruction_8) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_8_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &NonvirtualClassType = DBTH.getType("NonvirtualClass");
  EXPECT_NE(nullptr, NonvirtualClassType);
  const auto &NonvirtualStructType = DBTH.getType("NonvirtualStruct");
  EXPECT_NE(nullptr, NonvirtualStructType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));
  EXPECT_TRUE(DBTH.hasType(NonvirtualClassType));
  EXPECT_TRUE(DBTH.hasType(NonvirtualStructType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_9) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_9_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_10) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_10_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_11) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_11_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_12) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_12_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_12_b) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_12_b_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 3U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &ChildsChildType = DBTH.getType("ChildsChild");
  ASSERT_NE(nullptr, ChildsChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));
  EXPECT_TRUE(DBTH.hasType(ChildsChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
  const auto &SubTypesChild = DBTH.getSubTypes(ChildType);
  EXPECT_TRUE(SubTypesChild.find(ChildsChildType) != SubTypesChild.end());
}

TEST(DBTHTest, BasicTHReconstruction_12_c) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_12_c_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &ChildsChildType = DBTH.getType("ChildsChild");
  ASSERT_NE(nullptr, ChildsChildType);

  EXPECT_TRUE(DBTH.hasType(ChildType));
  EXPECT_TRUE(DBTH.hasType(ChildsChildType));

  // check for all subtypes
  const auto &SubTypesChild = DBTH.getSubTypes(ChildType);
  EXPECT_TRUE(SubTypesChild.find(ChildsChildType) != SubTypesChild.end());
}

/*
TEST(DBTHTest, BasicTHReconstruction_13) {
  Test file 13 has no types
}
*/

TEST(DBTHTest, BasicTHReconstruction_14) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_14_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 1U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);

  EXPECT_TRUE(DBTH.hasType(BaseType));

  // there are no subtypes here
}

TEST(DBTHTest, BasicTHReconstruction_15) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_15_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 2U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_16) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_16_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 5U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &ChildsChildType = DBTH.getType("ChildsChild");
  ASSERT_NE(nullptr, ChildsChildType);
  const auto &BaseTwoType = DBTH.getType("BaseTwo");
  ASSERT_NE(nullptr, BaseTwoType);
  const auto &ChildTwoType = DBTH.getType("ChildTwo");
  ASSERT_NE(nullptr, ChildTwoType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));
  EXPECT_TRUE(DBTH.hasType(ChildsChildType));
  EXPECT_TRUE(DBTH.hasType(BaseTwoType));
  EXPECT_TRUE(DBTH.hasType(ChildTwoType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
  const auto &SubTypesChild = DBTH.getSubTypes(ChildType);
  EXPECT_TRUE(SubTypesChild.find(ChildsChildType) != SubTypesChild.end());
  const auto &SubTypesTwo = DBTH.getSubTypes(BaseTwoType);
  EXPECT_TRUE(SubTypesTwo.find(ChildTwoType) != SubTypesTwo.end());
}

TEST(DBTHTest, BasicTHReconstruction_17) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_17_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 5U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &Child2Type = DBTH.getType("Child2");
  ASSERT_NE(nullptr, Child2Type);
  const auto &Base2Type = DBTH.getType("Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &KidType = DBTH.getType("Kid");
  ASSERT_NE(nullptr, KidType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));
  EXPECT_TRUE(DBTH.hasType(Child2Type));
  EXPECT_TRUE(DBTH.hasType(Base2Type));
  EXPECT_TRUE(DBTH.hasType(KidType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
  const auto &SubTypesChild = DBTH.getSubTypes(ChildType);
  EXPECT_TRUE(SubTypesChild.find(Child2Type) != SubTypesChild.end());
  const auto &SubTypesBase2 = DBTH.getSubTypes(Base2Type);
  EXPECT_TRUE(SubTypesBase2.find(KidType) != SubTypesBase2.end());
}

TEST(DBTHTest, BasicTHReconstruction_18) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_18_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 5U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &Child2Type = DBTH.getType("Child2");
  ASSERT_NE(nullptr, Child2Type);
  const auto &Child3Type = DBTH.getType("Child3");
  ASSERT_NE(nullptr, Child3Type);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));
  EXPECT_TRUE(DBTH.hasType(Child2Type));
  EXPECT_TRUE(DBTH.hasType(Child3Type));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
  const auto &SubTypesChild = DBTH.getSubTypes(ChildType);
  EXPECT_TRUE(SubTypesChild.find(Child2Type) != SubTypesChild.end());
  const auto &SubTypesChild2 = DBTH.getSubTypes(Child2Type);
  EXPECT_TRUE(SubTypesChild2.find(Child3Type) != SubTypesChild2.end());
}

TEST(DBTHTest, BasicTHReconstruction_19) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_19_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 6U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &FooType = DBTH.getType("Foo");
  ASSERT_NE(nullptr, FooType);
  const auto &BarType = DBTH.getType("Bar");
  ASSERT_NE(nullptr, BarType);
  const auto &LoremType = DBTH.getType("Lorem");
  ASSERT_NE(nullptr, LoremType);
  const auto &ImpsumType = DBTH.getType("Impsum");
  ASSERT_NE(nullptr, ImpsumType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));
  EXPECT_TRUE(DBTH.hasType(FooType));
  EXPECT_TRUE(DBTH.hasType(BarType));
  EXPECT_TRUE(DBTH.hasType(LoremType));
  EXPECT_TRUE(DBTH.hasType(ImpsumType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
  const auto &SubTypesFoo = DBTH.getSubTypes(FooType);
  EXPECT_TRUE(SubTypesFoo.find(BarType) != SubTypesFoo.end());
  const auto &SubTypesLorem = DBTH.getSubTypes(LoremType);
  EXPECT_TRUE(SubTypesLorem.find(ImpsumType) != SubTypesLorem.end());
}

TEST(DBTHTest, BasicTHReconstruction_20) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_20_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 3U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &Base2Type = DBTH.getType("Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(Base2Type));
  EXPECT_TRUE(DBTH.hasType(ChildType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
  const auto &SubTypes2 = DBTH.getSubTypes(Base2Type);
  EXPECT_TRUE(SubTypes2.find(ChildType) != SubTypes2.end());
}

TEST(DBTHTest, BasicTHReconstruction_21) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_21_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 5U);
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &Base2Type = DBTH.getType("Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &Base3Type = DBTH.getType("Base3");
  ASSERT_NE(nullptr, Base3Type);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &Child2Type = DBTH.getType("Child2");
  ASSERT_NE(nullptr, Child2Type);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(Base2Type));
  EXPECT_TRUE(DBTH.hasType(Base3Type));
  EXPECT_TRUE(DBTH.hasType(ChildType));
  EXPECT_TRUE(DBTH.hasType(Child2Type));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
  const auto &SubTypesBase2 = DBTH.getSubTypes(Base2Type);
  EXPECT_TRUE(SubTypesBase2.find(ChildType) != SubTypesBase2.end());
  const auto &SubTypesChild = DBTH.getSubTypes(ChildType);
  EXPECT_TRUE(SubTypesChild.find(Child2Type) != SubTypesChild.end());
  const auto &SubTypesBase3 = DBTH.getSubTypes(Base3Type);
  EXPECT_TRUE(SubTypesBase3.find(Child2Type) != SubTypesBase3.end());
}

/*
------------------------
VTableConstruction Tests
------------------------
*/

TEST(DBTHTest, VTableConstruction_1) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_1_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasVFTable(BaseType));
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);

  EXPECT_TRUE(VTableForChild->getFunction(0)->getName() == "_ZN5Child3fooEv");
}

TEST(DBTHTest, VTableConstruction_2) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_2_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
}

TEST(DBTHTest, VTableConstruction_3) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_3_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
}

TEST(DBTHTest, VTableConstruction_4) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_4_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
}

TEST(DBTHTest, VTableConstruction_5) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_5_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &OtherBaseType = DBTH.getType("OtherBase");
  ASSERT_NE(nullptr, OtherBaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
}

TEST(DBTHTest, VTableConstruction_6) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_6_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
}

TEST(DBTHTest, VTableConstruction_7) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_7_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &AType = DBTH.getType("A");
  ASSERT_NE(nullptr, AType);
  const auto &BType = DBTH.getType("B");
  ASSERT_NE(nullptr, BType);
  const auto &CType = DBTH.getType("C");
  ASSERT_NE(nullptr, CType);
  const auto &DType = DBTH.getType("D");
  ASSERT_NE(nullptr, DType);
  const auto &XType = DBTH.getType("X");
  ASSERT_NE(nullptr, XType);
  const auto &YType = DBTH.getType("Y");
  ASSERT_NE(nullptr, YType);
  const auto &ZType = DBTH.getType("Z");
  ASSERT_NE(nullptr, ZType);
}

TEST(DBTHTest, VTableConstruction_7_b) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_7_b_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &AType = DBTH.getType("A");
  ASSERT_NE(nullptr, AType);
  const auto &CType = DBTH.getType("C");
  ASSERT_NE(nullptr, CType);
  const auto &XType = DBTH.getType("X");
  ASSERT_NE(nullptr, XType);
  const auto &YType = DBTH.getType("Y");
  ASSERT_NE(nullptr, YType);
  const auto &ZType = DBTH.getType("Z");
  ASSERT_NE(nullptr, ZType);
  const auto &OmegaType = DBTH.getType("Omega");
  ASSERT_NE(nullptr, OmegaType);
}

TEST(DBTHTest, VTableConstruction_8) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_8_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &NonvirtualClassType = DBTH.getType("NonvirtualClass");
  EXPECT_NE(nullptr, NonvirtualClassType);
  const auto &NonvirtualStructType = DBTH.getType("NonvirtualStruct");
  EXPECT_NE(nullptr, NonvirtualStructType);
}

TEST(DBTHTest, VTableConstruction_9) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_9_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
}

TEST(DBTHTest, VTableConstruction_10) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_10_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
}

TEST(DBTHTest, VTableConstruction_11) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_11_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
}

TEST(DBTHTest, VTableConstruction_12) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_12_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
}

TEST(DBTHTest, VTableConstruction_12_b) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_12_b_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &ChildsChildType = DBTH.getType("ChildsChild");
  ASSERT_NE(nullptr, ChildsChildType);
}

TEST(DBTHTest, VTableConstruction_12_c) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_12_c_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &ChildsChildType = DBTH.getType("ChildsChild");
  ASSERT_NE(nullptr, ChildsChildType);
}

/*
TEST(DBTHTest, VTableConstruction_13) {
  Test file 13 has no types
}
*/

TEST(DBTHTest, VTableConstruction_14) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_14_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
}

TEST(DBTHTest, VTableConstruction_15) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_15_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
}

TEST(DBTHTest, VTableConstruction_16) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_16_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &ChildsChildType = DBTH.getType("ChildsChild");
  ASSERT_NE(nullptr, ChildsChildType);
  const auto &BaseTwoType = DBTH.getType("BaseTwo");
  ASSERT_NE(nullptr, BaseTwoType);
  const auto &ChildTwoType = DBTH.getType("ChildTwo");
  ASSERT_NE(nullptr, ChildTwoType);
}

TEST(DBTHTest, VTableConstruction_17) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_17_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &Child2Type = DBTH.getType("Child2");
  ASSERT_NE(nullptr, Child2Type);
  const auto &Base2Type = DBTH.getType("Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &KidType = DBTH.getType("Kid");
  ASSERT_NE(nullptr, KidType);
}

TEST(DBTHTest, VTableConstruction_18) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_18_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &Child2Type = DBTH.getType("Child2");
  ASSERT_NE(nullptr, Child2Type);
  const auto &Child3Type = DBTH.getType("Child3");
  ASSERT_NE(nullptr, Child3Type);
}

TEST(DBTHTest, VTableConstruction_19) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_19_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &FooType = DBTH.getType("Foo");
  ASSERT_NE(nullptr, FooType);
  const auto &BarType = DBTH.getType("Bar");
  ASSERT_NE(nullptr, BarType);
  const auto &LoremType = DBTH.getType("Lorem");
  ASSERT_NE(nullptr, LoremType);
  const auto &ImpsumType = DBTH.getType("Impsum");
  ASSERT_NE(nullptr, ImpsumType);
}

TEST(DBTHTest, VTableConstruction_20) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_20_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &Base2Type = DBTH.getType("Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
}

TEST(DBTHTest, VTableConstruction_21) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_21_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &Base2Type = DBTH.getType("Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &Base3Type = DBTH.getType("Base3");
  ASSERT_NE(nullptr, Base3Type);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &Child2Type = DBTH.getType("Child2");
  ASSERT_NE(nullptr, Child2Type);
}

/*
--------------------------------
TransitivelyReachableTypes Tests
--------------------------------
*/

TEST(DBTHTest, TransitivelyReachableTypes_1) {}
TEST(DBTHTest, TransitivelyReachableTypes_2) {}
TEST(DBTHTest, TransitivelyReachableTypes_3) {}
TEST(DBTHTest, TransitivelyReachableTypes_4) {}
TEST(DBTHTest, TransitivelyReachableTypes_5) {}
TEST(DBTHTest, TransitivelyReachableTypes_6) {}
TEST(DBTHTest, TransitivelyReachableTypes_7) {}
TEST(DBTHTest, TransitivelyReachableTypes_8) {}
TEST(DBTHTest, TransitivelyReachableTypes_9) {}
TEST(DBTHTest, TransitivelyReachableTypes_10) {}
TEST(DBTHTest, TransitivelyReachableTypes_11) {}
TEST(DBTHTest, TransitivelyReachableTypes_12) {}
TEST(DBTHTest, TransitivelyReachableTypes_12_a) {}
TEST(DBTHTest, TransitivelyReachableTypes_12_b) {}
/*
TEST(DBTHTest, BasicTHReconstruction_13) {
  Test file 13 has no types
}
*/
TEST(DBTHTest, TransitivelyReachableTypes_14) {}
TEST(DBTHTest, TransitivelyReachableTypes_15) {}
TEST(DBTHTest, TransitivelyReachableTypes_16) {}
TEST(DBTHTest, TransitivelyReachableTypes_17) {}
TEST(DBTHTest, TransitivelyReachableTypes_18) {}
TEST(DBTHTest, TransitivelyReachableTypes_19) {}
TEST(DBTHTest, TransitivelyReachableTypes_20) {}
TEST(DBTHTest, TransitivelyReachableTypes_21) {}

// Check basic type hierarchy construction
TEST(DBTHTest, BasicTHReconstruction_1) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_1_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &Types = DBTH.getAllTypes();
  const auto &SubTypes = DBTH.getSubTypes(DBTH.getType("Base"));

  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasVFTable(BaseType));

  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  EXPECT_TRUE(DBTH.hasType(ChildType));
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));
  const auto &VTableForChild = DBTH.getVFTable(ChildType);

  ASSERT_NE(nullptr, VTableForChild);
  EXPECT_TRUE(VTableForChild->getFunction(0)->getName() == "_ZN5Child3fooEv");
}

TEST(DBTHTest, BasicTHReconstruction_2) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_17_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasVFTable(BaseType));

  const auto &ChildType = DBTH.getType("Child");
  const auto &BaseSubTypes = DBTH.getSubTypes(DBTH.getType("Base"));
  ASSERT_NE(nullptr, ChildType);
  EXPECT_TRUE(DBTH.hasType(ChildType));
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));
  EXPECT_TRUE(BaseSubTypes.count(ChildType));

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  EXPECT_TRUE(VTableForChild->getFunction(0)->getName() == "_ZN5Child3fooEv");

  const auto &Base2Type = DBTH.getType("Base2");
  ASSERT_NE(nullptr, Base2Type);
  EXPECT_TRUE(DBTH.hasType(Base2Type));
  EXPECT_TRUE(DBTH.hasVFTable(Base2Type));

  const auto &KidType = DBTH.getType("Kid");
  ASSERT_NE(nullptr, KidType);
  EXPECT_TRUE(DBTH.hasType(KidType));
  ASSERT_TRUE(DBTH.hasVFTable(KidType));
  const auto &VTableForKid = DBTH.getVFTable(KidType);

  ASSERT_NE(nullptr, VTableForKid);
  EXPECT_TRUE(VTableForKid->getFunction(0)->getName() == "_ZN3Kid3fooEv");

  const auto &VTableForBase2 = DBTH.getVFTable(Base2Type);
  ASSERT_NE(nullptr, VTableForBase2);
  ASSERT_NE(nullptr, VTableForBase2->getFunction(1));
  EXPECT_TRUE(VTableForBase2->getFunction(1)->getName() == "_ZN5Base23barEv");
  ASSERT_NE(nullptr, VTableForBase2->getFunction(3));
  EXPECT_TRUE(VTableForBase2->getFunction(3)->getName() ==
              "_ZN5Base26foobarEv");
}

TEST(DBTHTest, BasicTHReconstruction_3) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_3_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &BaseType = DBTH.getType("Base");

  // ASSERT_NE(nullptr, BaseType);
  // EXPECT_TRUE(DBTH.hasType(BaseType));

  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  EXPECT_TRUE(DBTH.hasType(ChildType));

  EXPECT_TRUE(DBTH.isSuperType(ChildType, BaseType));

  // check VFTables

  ASSERT_TRUE(DBTH.hasVFTable(BaseType));
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);

  const auto &VTableForBaseFunction0 = VTableForBase->getFunction(0);
  ASSERT_NE(nullptr, VTableForBaseFunction0);
  EXPECT_TRUE(VTableForBaseFunction0->getName() == "_ZN4Base3fooEv");
  const auto &VTableForBaseFunction1 = VTableForBase->getFunction(1);
  ASSERT_NE(nullptr, VTableForBaseFunction1);
  EXPECT_TRUE(VTableForBaseFunction1->getName() == "_ZN4Base3barEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  const auto &VTableForChildFunction0 = VTableForChild->getFunction(0);
  ASSERT_NE(nullptr, VTableForChildFunction0);
  EXPECT_TRUE(VTableForChildFunction0->getName() == "_ZN5Child3fooEv");
  // Debug info doesn't include base bar() in the child function
}

TEST(DBTHTest, BasicTHReconstruction_4) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_18_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for types

  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  EXPECT_TRUE(DBTH.hasType(BaseType));

  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  EXPECT_TRUE(DBTH.hasType(ChildType));

  const auto &Child2Type = DBTH.getType("Child_2");
  ASSERT_NE(nullptr, Child2Type);
  EXPECT_TRUE(DBTH.hasType(Child2Type));

  const auto &Child3Type = DBTH.getType("Child_3");
  ASSERT_NE(nullptr, Child3Type);
  EXPECT_TRUE(DBTH.hasType(Child3Type));

  EXPECT_TRUE(DBTH.isSuperType(ChildType, BaseType));
  EXPECT_TRUE(DBTH.isSuperType(Child2Type, ChildType));
  EXPECT_TRUE(DBTH.isSuperType(Child3Type, Child2Type));

  // check VFTables

  ASSERT_TRUE(DBTH.hasVFTable(BaseType));
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));
  ASSERT_TRUE(DBTH.hasVFTable(Child2Type));
  EXPECT_TRUE(DBTH.hasVFTable(Child3Type));

  const auto &VTableForChild3 = DBTH.getVFTable(Child3Type);
  ASSERT_NE(nullptr, VTableForChild3);

  const auto &VTableForChild3Function3 = VTableForChild3->getFunction(3);
  ASSERT_NE(nullptr, VTableForChild3Function3);
  EXPECT_TRUE(VTableForChild3Function3->getName() == "_ZN7Child_36barfooEv");

  const auto &VTableForChild2 = DBTH.getVFTable(Child2Type);
  ASSERT_NE(nullptr, VTableForChild2);

  const auto &VTableForChild2Function2 = VTableForChild2->getFunction(2);
  ASSERT_NE(nullptr, VTableForChild2Function2);
  EXPECT_TRUE(VTableForChild2Function2->getName() == "_ZN7Child_26foobarEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  const auto &VTableForChildFunction0 = VTableForChild->getFunction(0);
  ASSERT_NE(nullptr, VTableForChildFunction0);
  EXPECT_TRUE(VTableForChildFunction0->getName() == "_ZN5Child3fooEv");

  // subtypes
  const auto &BaseSubTypes = DBTH.getSubTypes(BaseType);
  ASSERT_TRUE(!BaseSubTypes.empty());
  EXPECT_TRUE(BaseSubTypes.find(ChildType) != BaseSubTypes.end());
  EXPECT_TRUE(BaseSubTypes.find(Child2Type) != BaseSubTypes.end());
  EXPECT_TRUE(BaseSubTypes.find(Child3Type) != BaseSubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_5) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_16_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for types

  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  EXPECT_TRUE(DBTH.hasType(BaseType));

  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  EXPECT_TRUE(DBTH.hasType(ChildType));

  const auto &ChildOfChildType = DBTH.getType("ChildOfChild");
  ASSERT_NE(nullptr, ChildOfChildType);
  EXPECT_TRUE(DBTH.hasType(ChildOfChildType));

  const auto &BaseTwoType = DBTH.getType("BaseTwo");
  ASSERT_NE(nullptr, BaseTwoType);
  EXPECT_TRUE(DBTH.hasType(BaseTwoType));

  const auto &ChildTwoType = DBTH.getType("ChildTwo");
  ASSERT_NE(nullptr, ChildTwoType);
  EXPECT_TRUE(DBTH.hasType(ChildTwoType));

  EXPECT_TRUE(DBTH.isSuperType(ChildType, BaseType));
  EXPECT_TRUE(DBTH.isSuperType(ChildOfChildType, ChildType));
  EXPECT_TRUE(DBTH.isSuperType(ChildTwoType, BaseTwoType));

  // check VFTables

  ASSERT_TRUE(DBTH.hasVFTable(BaseType));
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));
  EXPECT_FALSE(DBTH.hasVFTable(ChildOfChildType));
  ASSERT_TRUE(DBTH.hasVFTable(BaseTwoType));
  ASSERT_TRUE(DBTH.hasVFTable(ChildTwoType));

  EXPECT_EQ(DBTH.getVFTable(DBTH.getType("Base"))->getFunction(0)->getName(),
            "_ZN4Base3fooEv");
  EXPECT_EQ(DBTH.getVFTable(DBTH.getType("Child"))->getFunction(0)->getName(),
            "_ZN5Child3fooEv");
  EXPECT_EQ(DBTH.getVFTable(DBTH.getType("BaseTwo"))->getFunction(0)->getName(),
            "_ZN7BaseTwo6foobarEv");
  EXPECT_EQ(
      DBTH.getVFTable(DBTH.getType("ChildTwo"))->getFunction(0)->getName(),
      "_ZN8ChildTwo6foobarEv");
}

// check if the vtables are constructed correctly in more complex scenarios
TEST(LTHTest, VTableConstruction) {
  LLVMProjectIRDB IRDB1({unittest::PathToLLTestFiles +
                         "type_hierarchies/type_hierarchy_1_cpp_dbg.ll"});
  LLVMProjectIRDB IRDB2({unittest::PathToLLTestFiles +
                         "type_hierarchies/type_hierarchy_7_cpp_dbg.ll"});
  LLVMProjectIRDB IRDB3({unittest::PathToLLTestFiles +
                         "type_hierarchies/type_hierarchy_8_cpp_dbg.ll"});
  LLVMProjectIRDB IRDB4({unittest::PathToLLTestFiles +
                         "type_hierarchies/type_hierarchy_9_cpp_dbg.ll"});
  LLVMProjectIRDB IRDB5({unittest::PathToLLTestFiles +
                         "type_hierarchies/type_hierarchy_10_cpp_dbg.ll"});
  LLVMProjectIRDB IRDB6({unittest::PathToLLTestFiles +
                         "type_hierarchies/type_hierarchy_14_cpp_dbg.ll"});

  DIBasedTypeHierarchy TH3(IRDB3);
  DIBasedTypeHierarchy TH4(IRDB4);
  DIBasedTypeHierarchy TH5(IRDB5);
  DIBasedTypeHierarchy TH6(IRDB6);

  // TH1
  DIBasedTypeHierarchy TH1(IRDB1);
  ASSERT_TRUE(TH1.getType("Base"));
  ASSERT_TRUE(TH1.hasVFTable(TH1.getType("Base")));
  ASSERT_TRUE(TH1.getVFTable(TH1.getType("Base"))->getFunction(0));
  EXPECT_EQ(TH1.getVFTable(TH1.getType("Base"))->getFunction(0)->getName(),
            "_ZN4Base3fooEv");
  EXPECT_EQ(TH1.getVFTable(TH1.getType("Base"))->size(), 1U);

  ASSERT_TRUE(TH1.getType("Child"));
  ASSERT_TRUE(TH1.hasVFTable(TH1.getType("Child")));
  ASSERT_TRUE(TH1.getVFTable(TH1.getType("Child"))->getFunction(0));
  EXPECT_EQ(TH1.getVFTable(TH1.getType("Child"))->getFunction(0)->getName(),
            "_ZN5Child3fooEv");
  EXPECT_EQ(TH1.getVFTable(TH1.getType("Child"))->size(), 1U);

  // TH2
  DIBasedTypeHierarchy TH2(IRDB2);
  ASSERT_TRUE(TH2.getType("A"));
  ASSERT_TRUE(TH2.hasVFTable(TH2.getType("A")));
  ASSERT_TRUE(TH2.getVFTable(TH2.getType("A"))->getFunction(0));
  EXPECT_EQ(TH2.getVFTable(TH2.getType("A"))->getFunction(0)->getName(),
            "_ZN1A1fEv");

  ASSERT_TRUE(TH2.getType("X"));
  ASSERT_TRUE(TH2.hasVFTable(TH2.getType("X")));
  ASSERT_TRUE(TH2.getVFTable(TH2.getType("X"))->getFunction(0));
  EXPECT_EQ(TH2.getVFTable(TH2.getType("X"))->getFunction(0)->getName(),
            "_ZN1X1gEv");

  EXPECT_FALSE(TH2.hasVFTable(TH2.getType("B")));
  EXPECT_FALSE(TH2.hasVFTable(TH2.getType("C")));
  EXPECT_FALSE(TH2.hasVFTable(TH2.getType("D")));
  EXPECT_FALSE(TH2.hasVFTable(TH2.getType("Y")));
  EXPECT_FALSE(TH2.hasVFTable(TH2.getType("Z")));

  // TH3
  ASSERT_TRUE(TH3.getType("Base"));
  ASSERT_TRUE(TH3.hasVFTable(TH3.getType("Base")));
  EXPECT_TRUE(TH3.getVFTable(TH3.getType("Base"))->getFunction(0));
  ASSERT_TRUE(TH3.getType("Child"));
  ASSERT_TRUE(TH3.hasVFTable(TH3.getType("Child")));
  EXPECT_TRUE(TH3.getVFTable(TH3.getType("Child"))->getFunction(0));

  EXPECT_EQ(TH3.getVFTable(TH3.getType("Base"))->getFunction(0)->getName(),
            "_ZN4Base3fooEv");
  EXPECT_EQ(TH3.getVFTable(TH3.getType("Base"))->getFunction(1)->getName(),
            "_ZN4Base3barEv");
  EXPECT_TRUE(TH3.getVFTable(TH3.getType("Base"))->size() == 2U);
  EXPECT_EQ(TH3.getVFTable(TH3.getType("Child"))->getFunction(0)->getName(),
            "_ZN5Child3fooEv");
  EXPECT_EQ(TH3.getVFTable(TH3.getType("Child"))->getFunction(2)->getName(),
            "_ZN5Child3bazEv");
  EXPECT_TRUE(TH3.getVFTable(TH3.getType("Child"))->size() == 3U);

  EXPECT_FALSE(TH3.hasVFTable(TH3.getType("NonvirtualClass")));
  EXPECT_FALSE(TH3.hasVFTable(TH3.getType("NonvirtualStruct")));

  // TH4
  ASSERT_TRUE(TH4.getType("Base"));
  ASSERT_TRUE(TH4.hasVFTable(TH4.getType("Base")));
  ASSERT_TRUE(TH4.getVFTable(TH4.getType("Base"))->getFunction(0));
  ASSERT_TRUE(TH4.getType("Child"));
  ASSERT_TRUE(TH4.hasVFTable(TH4.getType("Child")));
  ASSERT_TRUE(TH4.getVFTable(TH4.getType("Child"))->getFunction(0));

  EXPECT_EQ(TH4.getVFTable(TH4.getType("Base"))->getFunction(0)->getName(),
            "_ZN4Base3fooEv");
  EXPECT_EQ(TH4.getVFTable(TH4.getType("Base"))->getFunction(1)->getName(),
            "_ZN4Base3barEv");
  EXPECT_TRUE(TH4.getVFTable(TH4.getType("Base"))->size() == 2U);
  EXPECT_EQ(TH4.getVFTable(TH4.getType("Child"))->getFunction(0)->getName(),
            "_ZN5Child3fooEv");
  EXPECT_EQ(TH4.getVFTable(TH4.getType("Child"))->getFunction(2)->getName(),
            "_ZN5Child3bazEv");
  EXPECT_TRUE(TH4.getVFTable(TH4.getType("Child"))->size() == 3U);

  // TH5
  ASSERT_TRUE(TH5.getType("Base"));
  ASSERT_TRUE(TH5.hasVFTable(TH5.getType("Base")));
  ASSERT_TRUE(TH5.getVFTable(TH5.getType("Base"))->getFunction(1));

  ASSERT_TRUE(TH5.getType("Child"));
  ASSERT_TRUE(TH5.hasVFTable(TH5.getType("Child")));
  ASSERT_TRUE(TH5.getVFTable(TH5.getType("Child"))->getFunction(0));
  ASSERT_TRUE(TH5.getVFTable(TH5.getType("Child"))->getFunction(2));

  EXPECT_EQ(TH5.getVFTable(TH5.getType("Base"))->getFunction(1)->getName(),
            "_ZN4Base3barEv");
  EXPECT_EQ(TH5.getVFTable(TH5.getType("Child"))->getFunction(0)->getName(),
            "_ZN5Child3fooEv");
  EXPECT_EQ(TH5.getVFTable(TH5.getType("Child"))->getFunction(2)->getName(),
            "_ZN5Child3bazEv");
}

TEST(LTHTest, TransitivelyReachableTypes) {
  LLVMProjectIRDB IRDB1({unittest::PathToLLTestFiles +
                         "type_hierarchies/type_hierarchy_1_cpp_dbg.ll"});
  LLVMProjectIRDB IRDB2({unittest::PathToLLTestFiles +
                         "type_hierarchies/type_hierarchy_7_cpp_dbg.ll"});
  LLVMProjectIRDB IRDB3({unittest::PathToLLTestFiles +
                         "type_hierarchies/type_hierarchy_8_cpp_dbg.ll"});
  LLVMProjectIRDB IRDB4({unittest::PathToLLTestFiles +
                         "type_hierarchies/type_hierarchy_9_cpp_dbg.ll"});
  LLVMProjectIRDB IRDB5({unittest::PathToLLTestFiles +
                         "type_hierarchies/type_hierarchy_10_cpp_dbg.ll"});
  // Creates an empty type hierarchy
  DIBasedTypeHierarchy TH1(IRDB1);
  DIBasedTypeHierarchy TH2(IRDB2);
  DIBasedTypeHierarchy TH3(IRDB3);
  DIBasedTypeHierarchy TH4(IRDB4);
  DIBasedTypeHierarchy TH5(IRDB5);

  auto ReachableTypesBase1 = TH1.getSubTypes(TH1.getType("Base"));
  auto ReachableTypesChild1 = TH1.getSubTypes(TH1.getType("Child"));

  auto ReachableTypesA2 = TH2.getSubTypes(TH2.getType("A"));
  auto ReachableTypesB2 = TH2.getSubTypes(TH2.getType("B"));
  auto ReachableTypesC2 = TH2.getSubTypes(TH2.getType("C"));
  auto ReachableTypesD2 = TH2.getSubTypes(TH2.getType("D"));
  auto ReachableTypesX2 = TH2.getSubTypes(TH2.getType("X"));
  auto ReachableTypesY2 = TH2.getSubTypes(TH2.getType("Y"));
  auto ReachableTypesZ2 = TH2.getSubTypes(TH2.getType("Z"));

  auto ReachableTypesBase3 = TH3.getSubTypes(TH3.getType("Base"));
  auto ReachableTypesChild3 = TH3.getSubTypes(TH3.getType("Child"));
  auto ReachableTypesNonvirtualclass3 =
      TH3.getSubTypes(TH3.getType("NonvirtualClass"));
  auto ReachableTypesNonvirtualstruct3 =
      TH3.getSubTypes(TH3.getType("NonvirtualStruct"));

  auto ReachableTypesBase4 = TH4.getSubTypes(TH4.getType("Base"));
  auto ReachableTypesChild4 = TH4.getSubTypes(TH4.getType("Child"));

  auto ReachableTypesBase5 = TH5.getSubTypes(TH5.getType("Base"));
  auto ReachableTypesChild5 = TH5.getSubTypes(TH5.getType("Child"));

  // Will be way less dangerous to have an interface (like a map) between the
  // llvm given name of class & struct (i.e. Base.base ...) and the name
  // inside phasar (i.e. just Base) and never work with the llvm name inside
  // phasar
  EXPECT_TRUE(ReachableTypesBase1.count(TH1.getType("Base")));
  EXPECT_TRUE(ReachableTypesBase1.count(TH1.getType("Child")));
  EXPECT_TRUE(ReachableTypesBase1.size() == 2U);
  EXPECT_FALSE(ReachableTypesChild1.count(TH1.getType("Base")));
  EXPECT_TRUE(ReachableTypesChild1.count(TH1.getType("Child")));
  EXPECT_TRUE(ReachableTypesChild1.size() == 1U);

  EXPECT_TRUE(ReachableTypesA2.count(TH2.getType("A")));
  EXPECT_TRUE(ReachableTypesA2.count(TH2.getType("B")));
  EXPECT_TRUE(ReachableTypesA2.count(TH2.getType("C")));
  EXPECT_TRUE(ReachableTypesA2.count(TH2.getType("D")));
  EXPECT_TRUE(ReachableTypesA2.count(TH2.getType("Z")));
  EXPECT_EQ(ReachableTypesA2.size(), 5U);
  EXPECT_TRUE(ReachableTypesB2.count(TH2.getType("B")));
  EXPECT_TRUE(ReachableTypesB2.count(TH2.getType("D")));
  EXPECT_EQ(ReachableTypesB2.size(), 2U);
  EXPECT_TRUE(ReachableTypesC2.count(TH2.getType("C")));
  EXPECT_TRUE(ReachableTypesC2.count(TH2.getType("Z")));
  EXPECT_TRUE(ReachableTypesC2.size() == 2U);
  EXPECT_TRUE(ReachableTypesD2.count(TH2.getType("D")));
  EXPECT_TRUE(ReachableTypesD2.size() == 1U);
  EXPECT_TRUE(ReachableTypesX2.count(TH2.getType("X")));
  EXPECT_TRUE(ReachableTypesX2.count(TH2.getType("Y")));
  EXPECT_TRUE(ReachableTypesX2.count(TH2.getType("Z")));
  EXPECT_TRUE(ReachableTypesX2.size() == 3U);
  EXPECT_TRUE(ReachableTypesY2.count(TH2.getType("Y")));
  EXPECT_TRUE(ReachableTypesY2.count(TH2.getType("Z")));
  EXPECT_TRUE(ReachableTypesY2.size() == 2U);
  EXPECT_TRUE(ReachableTypesZ2.count(TH2.getType("Z")));
  EXPECT_TRUE(ReachableTypesZ2.size() == 1U);

  EXPECT_TRUE(ReachableTypesBase3.count(TH3.getType("Base")));
  EXPECT_TRUE(ReachableTypesBase3.count(TH3.getType("Child")));
  EXPECT_TRUE(ReachableTypesBase3.size() == 2U);
  EXPECT_TRUE(ReachableTypesChild3.count(TH3.getType("Child")));
  EXPECT_TRUE(ReachableTypesChild3.size() == 1U);
  EXPECT_TRUE(
      ReachableTypesNonvirtualclass3.count(TH3.getType("NonvirtualClass")));
  EXPECT_TRUE(ReachableTypesNonvirtualclass3.size() == 1U);
  EXPECT_TRUE(
      ReachableTypesNonvirtualstruct3.count(TH3.getType("NonvirtualStruct")));
  EXPECT_TRUE(ReachableTypesNonvirtualstruct3.size() == 1U);

  EXPECT_TRUE(ReachableTypesBase4.count(TH4.getType("Base")));
  EXPECT_TRUE(ReachableTypesBase4.count(TH4.getType("Child")));
  EXPECT_TRUE(ReachableTypesBase4.size() == 2U);
  EXPECT_TRUE(ReachableTypesChild4.count(TH4.getType("Child")));
  EXPECT_TRUE(ReachableTypesChild4.size() == 1U);

  EXPECT_TRUE(ReachableTypesBase5.count(TH5.getType("Base")));
  EXPECT_TRUE(ReachableTypesBase5.count(TH5.getType("Child")));
  EXPECT_TRUE(ReachableTypesBase5.size() == 2U);
  EXPECT_TRUE(ReachableTypesChild5.count(TH5.getType("Child")));
  EXPECT_TRUE(ReachableTypesChild5.size() == 1U);
}

} // namespace psr

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  auto Res = RUN_ALL_TESTS();
  return Res;
}
