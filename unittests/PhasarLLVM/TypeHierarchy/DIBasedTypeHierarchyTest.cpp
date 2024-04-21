
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &OtherBaseType = DBTH.getType("_ZTS9OtherBase");
  ASSERT_NE(nullptr, OtherBaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
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
  const auto &AType = DBTH.getType("_ZTS1A");
  ASSERT_NE(nullptr, AType);
  const auto &BType = DBTH.getType("_ZTS1B");
  ASSERT_NE(nullptr, BType);
  const auto &CType = DBTH.getType("_ZTS1C");
  ASSERT_NE(nullptr, CType);
  const auto &DType = DBTH.getType("_ZTS1D");
  ASSERT_NE(nullptr, DType);
  const auto &XType = DBTH.getType("_ZTS1X");
  ASSERT_NE(nullptr, XType);
  const auto &YType = DBTH.getType("_ZTS1Y");
  ASSERT_NE(nullptr, YType);
  const auto &ZType = DBTH.getType("_ZTS1Z");
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
  EXPECT_EQ(DBTH.getAllTypes().size(), 6U);
  const auto &AType = DBTH.getType("A");
  ASSERT_NE(nullptr, AType);
  const auto &CType = DBTH.getType("_ZTS1C");
  ASSERT_NE(nullptr, CType);
  const auto &XType = DBTH.getType("X");
  ASSERT_NE(nullptr, XType);
  const auto &YType = DBTH.getType("_ZTS1Y");
  ASSERT_NE(nullptr, YType);
  const auto &ZType = DBTH.getType("_ZTS1Z");
  ASSERT_NE(nullptr, ZType);
  const auto &OmegaType = DBTH.getType("_ZTS5Omega");
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
  EXPECT_EQ(DBTH.getAllTypes().size(), 4U);
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &NonvirtualClassType = DBTH.getType("_ZTS15NonvirtualClass");
  EXPECT_NE(nullptr, NonvirtualClassType);
  const auto &NonvirtualStructType = DBTH.getType("_ZTS16NonvirtualStruct");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
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
  const auto &ChildsChildType = DBTH.getType("_ZTS11ChildsChild");
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
  const auto &ChildsChildType = DBTH.getType("_ZTS11ChildsChild");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  // Since ChildsChild is never used, it is optimized out
  // const auto &ChildsChildType = DBTH.getType("ChildsChild");
  // ASSERT_EQ(nullptr, ChildsChildType);
  const auto &BaseTwoType = DBTH.getType("_ZTS7BaseTwo");
  ASSERT_NE(nullptr, BaseTwoType);
  const auto &ChildTwoType = DBTH.getType("_ZTS8ChildTwo");
  ASSERT_NE(nullptr, ChildTwoType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));
  // Since ChildsChild is never used, it is optimized out
  // EXPECT_FALSE(DBTH.hasType(ChildsChildType));
  EXPECT_TRUE(DBTH.hasType(BaseTwoType));
  EXPECT_TRUE(DBTH.hasType(ChildTwoType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
  // const auto &SubTypesChild = DBTH.getSubTypes(ChildType);
  //  Since ChildsChild is never used, it is optimized out
  //  EXPECT_TRUE(SubTypesChild.find(ChildsChildType) == SubTypesChild.end());
  const auto &SubTypesTwo = DBTH.getSubTypes(BaseTwoType);
  EXPECT_TRUE(SubTypesTwo.find(ChildTwoType) != SubTypesTwo.end());
}

TEST(DBTHTest, BasicTHReconstruction_17) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_17_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  // EXPECT_EQ(DBTH.getAllTypes().size(), 5U);
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  // const auto &Child2Type = DBTH.getType("Child2");
  // Since Child2Type is never used, it is optimized out
  // ASSERT_EQ(nullptr, Child2Type);
  const auto &Base2Type = DBTH.getType("_ZTS5Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &KidType = DBTH.getType("_ZTS3Kid");
  ASSERT_NE(nullptr, KidType);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));
  // Since ChildsChild is never used, it is optimized out
  // EXPECT_FALSE(DBTH.hasType(Child2Type));
  EXPECT_TRUE(DBTH.hasType(Base2Type));
  EXPECT_TRUE(DBTH.hasType(KidType));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
  // const auto &SubTypesChild = DBTH.getSubTypes(ChildType);
  // Since ChildsChild is never used, it is optimized out
  // EXPECT_TRUE(SubTypesChild.find(Child2Type) == SubTypesChild.end());
  const auto &SubTypesBase2 = DBTH.getSubTypes(Base2Type);
  EXPECT_TRUE(SubTypesBase2.find(KidType) != SubTypesBase2.end());
}

TEST(DBTHTest, BasicTHReconstruction_18) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_18_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 4U);
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  // const auto &Child_2Type = DBTH.getType("Child_2");
  //  Since Child2Type is never used, it is optimized out
  //  ASSERT_EQ(nullptr, Child2Type);
  const auto &Child3Type = DBTH.getType("_ZTS7Child_3");
  ASSERT_NE(nullptr, Child3Type);

  EXPECT_TRUE(DBTH.hasType(BaseType));
  EXPECT_TRUE(DBTH.hasType(ChildType));
  // Since Child2 is never used, it is optimized out
  // EXPECT_FALSE(DBTH.hasType(Child2Type));
  EXPECT_TRUE(DBTH.hasType(Child3Type));

  // check for all subtypes
  const auto &SubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());
  // const auto &SubTypesChild = DBTH.getSubTypes(ChildType);
  // Since Child2 is never used, it is optimized out
  // EXPECT_TRUE(SubTypesChild.find(Child2Type) == SubTypesChild.end());
  const auto &SubTypesChild2 = DBTH.getSubTypes(Child3Type);
  EXPECT_TRUE(SubTypesChild2.find(Child3Type) != SubTypesChild2.end());
}

TEST(DBTHTest, BasicTHReconstruction_19) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_19_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  EXPECT_EQ(DBTH.getAllTypes().size(), 6U);
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &FooType = DBTH.getType("_ZTS3Foo");
  ASSERT_NE(nullptr, FooType);
  const auto &BarType = DBTH.getType("_ZTS3Bar");
  ASSERT_NE(nullptr, BarType);
  const auto &LoremType = DBTH.getType("_ZTS5Lorem");
  ASSERT_NE(nullptr, LoremType);
  const auto &ImpsumType = DBTH.getType("_ZTS6Impsum");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &Base2Type = DBTH.getType("_ZTS5Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &Base2Type = DBTH.getType("_ZTS5Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &Base3Type = DBTH.getType("_ZTS5Base3");
  ASSERT_NE(nullptr, Base3Type);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &Child2Type = DBTH.getType("_ZTS6Child2");
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
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  ASSERT_TRUE(DBTH.hasVFTable(BaseType));
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base3fooEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
}

TEST(DBTHTest, VTableConstruction_2) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_2_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  ASSERT_TRUE(DBTH.hasVFTable(BaseType));
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base3fooEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
}

TEST(DBTHTest, VTableConstruction_3) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_3_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  ASSERT_TRUE(DBTH.hasVFTable(BaseType));
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  ASSERT_NE(nullptr, VTableForBase->getFunction(1));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base3fooEv");
  EXPECT_EQ(VTableForBase->getFunction(1)->getName(), "_ZN4Base3barEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
}

TEST(DBTHTest, VTableConstruction_4) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_4_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  ASSERT_TRUE(DBTH.hasVFTable(BaseType));
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  ASSERT_NE(nullptr, VTableForBase->getFunction(1));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base3fooEv");
  EXPECT_EQ(VTableForBase->getFunction(1)->getName(), "_ZN4Base3barEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_TRUE(VTableForChild->getFunction(1) == nullptr);
  ASSERT_NE(nullptr, VTableForChild->getFunction(2));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
  EXPECT_EQ(VTableForChild->getFunction(2)->getName(), "_ZN5Child3tarEv");
}

TEST(DBTHTest, VTableConstruction_5) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_5_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &OtherBaseType = DBTH.getType("_ZTS9OtherBase");
  ASSERT_NE(nullptr, OtherBaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  ASSERT_TRUE(DBTH.hasVFTable(BaseType));
  ASSERT_TRUE(DBTH.hasVFTable(OtherBaseType));
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  ASSERT_NE(nullptr, VTableForBase->getFunction(1));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base3fooEv");
  EXPECT_EQ(VTableForBase->getFunction(1)->getName(), "_ZN4Base3barEv");

  const auto &VTableForOtherBase = DBTH.getVFTable(OtherBaseType);
  ASSERT_NE(nullptr, VTableForOtherBase);
  ASSERT_NE(nullptr, VTableForOtherBase->getFunction(0));
  EXPECT_EQ(VTableForOtherBase->getFunction(0)->getName(),
            "_ZN9OtherBase3bazEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_TRUE(VTableForChild->getFunction(1) == nullptr);
  ASSERT_NE(nullptr, VTableForChild->getFunction(2));
  ASSERT_NE(nullptr, VTableForChild->getFunction(3));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
  EXPECT_EQ(VTableForChild->getFunction(2)->getName(), "_ZN5Child3bazEv");
  EXPECT_EQ(VTableForChild->getFunction(3)->getName(), "_ZN5Child3tarEv");
}

TEST(DBTHTest, VTableConstruction_6) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_6_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  ASSERT_NE(nullptr, VTableForBase->getFunction(1));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base3fooEv");
  EXPECT_EQ(VTableForBase->getFunction(1)->getName(), "_ZN4Base3barEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_TRUE(VTableForChild->getFunction(1) == nullptr);
  ASSERT_NE(nullptr, VTableForChild->getFunction(2));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
  EXPECT_EQ(VTableForChild->getFunction(2)->getName(), "_ZN5Child3tarEv");
}

TEST(DBTHTest, VTableConstruction_7) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_7_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &AType = DBTH.getType("_ZTS1A");
  ASSERT_NE(nullptr, AType);
  const auto &BType = DBTH.getType("_ZTS1B");
  ASSERT_NE(nullptr, BType);
  const auto &CType = DBTH.getType("_ZTS1C");
  ASSERT_NE(nullptr, CType);
  const auto &DType = DBTH.getType("_ZTS1D");
  ASSERT_NE(nullptr, DType);
  const auto &XType = DBTH.getType("_ZTS1X");
  ASSERT_NE(nullptr, XType);
  const auto &YType = DBTH.getType("_ZTS1Y");
  ASSERT_NE(nullptr, YType);
  const auto &ZType = DBTH.getType("_ZTS1Z");
  ASSERT_NE(nullptr, ZType);

  const auto &VTableForAType = DBTH.getVFTable(AType);
  ASSERT_NE(nullptr, VTableForAType);
  ASSERT_NE(nullptr, VTableForAType->getFunction(0));
  EXPECT_EQ(VTableForAType->getFunction(0)->getName(), "_ZN1A1fEv");
  const auto &VTableForBType = DBTH.getVFTable(BType);
  ASSERT_NE(nullptr, VTableForBType);
  EXPECT_EQ(VTableForBType->getFunction(0), nullptr);
  const auto &VTableForCType = DBTH.getVFTable(CType);
  ASSERT_NE(nullptr, VTableForCType);
  EXPECT_EQ(VTableForCType->getFunction(0), nullptr);
  const auto &VTableForDType = DBTH.getVFTable(DType);
  ASSERT_NE(nullptr, VTableForDType);
  EXPECT_EQ(VTableForDType->getFunction(0), nullptr);
  const auto &VTableForXType = DBTH.getVFTable(XType);
  ASSERT_NE(nullptr, VTableForXType);
  ASSERT_NE(nullptr, VTableForXType->getFunction(0));
  EXPECT_EQ(VTableForXType->getFunction(0)->getName(), "_ZN1X1gEv");
  const auto &VTableForYType = DBTH.getVFTable(YType);
  ASSERT_NE(nullptr, VTableForYType);
  EXPECT_EQ(VTableForYType->getFunction(0), nullptr);
  const auto &VTableForZType = DBTH.getVFTable(ZType);
  ASSERT_NE(nullptr, VTableForZType);
  EXPECT_EQ(VTableForZType->getFunction(0), nullptr);
}

TEST(DBTHTest, VTableConstruction_7_b) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_7_b_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &AType = DBTH.getType("A");
  ASSERT_NE(nullptr, AType);
  const auto &CType = DBTH.getType("_ZTS1C");
  ASSERT_NE(nullptr, CType);
  const auto &XType = DBTH.getType("X");
  ASSERT_NE(nullptr, XType);
  const auto &YType = DBTH.getType("_ZTS1Y");
  ASSERT_NE(nullptr, YType);
  const auto &ZType = DBTH.getType("_ZTS1Z");
  ASSERT_NE(nullptr, ZType);
  const auto &OmegaType = DBTH.getType("_ZTS5Omega");
  ASSERT_NE(nullptr, OmegaType);

  const auto &VTableForAType = DBTH.getVFTable(AType);
  ASSERT_NE(nullptr, VTableForAType);
  ASSERT_NE(nullptr, VTableForAType->getFunction(0));
  EXPECT_EQ(VTableForAType->getFunction(0)->getName(), "_ZN1A1fEv");
  const auto &VTableForCType = DBTH.getVFTable(CType);
  ASSERT_NE(nullptr, VTableForCType);
  EXPECT_EQ(VTableForCType->getFunction(0), nullptr);
  const auto &VTableForXType = DBTH.getVFTable(XType);
  ASSERT_NE(nullptr, VTableForXType);
  ASSERT_NE(nullptr, VTableForXType->getFunction(0));
  EXPECT_EQ(VTableForXType->getFunction(0)->getName(), "_ZN1X1gEv");
  const auto &VTableForYType = DBTH.getVFTable(YType);
  ASSERT_NE(nullptr, VTableForYType);
  EXPECT_EQ(VTableForYType->getFunction(0), nullptr);
  const auto &VTableForZType = DBTH.getVFTable(ZType);
  ASSERT_NE(nullptr, VTableForZType);
  EXPECT_EQ(VTableForZType->getFunction(0), nullptr);
  const auto &VTableForOmegaType = DBTH.getVFTable(OmegaType);
  ASSERT_NE(nullptr, VTableForOmegaType);
  ASSERT_NE(nullptr, VTableForOmegaType->getFunction(0));
  EXPECT_EQ(VTableForOmegaType->getFunction(0)->getName(), "_ZN5Omega1fEv");
}

TEST(DBTHTest, VTableConstruction_8) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_8_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &NonvirtualClassType = DBTH.getType("_ZTS15NonvirtualClass");
  EXPECT_NE(nullptr, NonvirtualClassType);
  const auto &NonvirtualStructType = DBTH.getType("_ZTS16NonvirtualStruct");
  EXPECT_NE(nullptr, NonvirtualStructType);

  const auto &VTableForBaseType = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBaseType);
  ASSERT_NE(nullptr, VTableForBaseType->getFunction(0));
  ASSERT_NE(nullptr, VTableForBaseType->getFunction(1));
  EXPECT_EQ(VTableForBaseType->getFunction(0)->getName(), "_ZN4Base3fooEv");
  EXPECT_EQ(VTableForBaseType->getFunction(1)->getName(), "_ZN4Base3barEv");
  const auto &VTableForChildType = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChildType);
  ASSERT_NE(nullptr, VTableForChildType->getFunction(0));
  EXPECT_EQ(VTableForChildType->getFunction(1), nullptr);
  ASSERT_NE(nullptr, VTableForChildType->getFunction(2));
  EXPECT_EQ(VTableForChildType->getFunction(0)->getName(), "_ZN5Child3fooEv");
  EXPECT_EQ(VTableForChildType->getFunction(2)->getName(), "_ZN5Child3bazEv");

  const auto &VTableForNonvirtualClassType =
      DBTH.getVFTable(NonvirtualClassType);
  ASSERT_NE(nullptr, VTableForNonvirtualClassType);
  EXPECT_EQ(VTableForNonvirtualClassType->getFunction(0), nullptr);
  const auto &VTableForNonvirtualStructType =
      DBTH.getVFTable(NonvirtualStructType);
  ASSERT_NE(nullptr, VTableForNonvirtualStructType);
  EXPECT_EQ(VTableForNonvirtualStructType->getFunction(0), nullptr);
}

TEST(DBTHTest, VTableConstruction_9) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_9_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  ASSERT_NE(nullptr, VTableForBase->getFunction(1));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base3fooEv");
  EXPECT_EQ(VTableForBase->getFunction(1)->getName(), "_ZN4Base3barEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_TRUE(VTableForChild->getFunction(1) == nullptr);
  ASSERT_NE(nullptr, VTableForChild->getFunction(2));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
  EXPECT_EQ(VTableForChild->getFunction(2)->getName(), "_ZN5Child3bazEv");
}

TEST(DBTHTest, VTableConstruction_10) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_10_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  EXPECT_EQ(VTableForBase->getFunction(0), nullptr);
  ASSERT_NE(nullptr, VTableForBase->getFunction(1));
  EXPECT_EQ(VTableForBase->getFunction(1)->getName(), "_ZN4Base3barEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_TRUE(VTableForChild->getFunction(1) == nullptr);
  ASSERT_NE(nullptr, VTableForChild->getFunction(2));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
  EXPECT_EQ(VTableForChild->getFunction(2)->getName(), "_ZN5Child3bazEv");
}

TEST(DBTHTest, VTableConstruction_11) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_11_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base3fooEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
}

TEST(DBTHTest, VTableConstruction_12) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_12_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base3fooEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
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
  const auto &ChildsChildType = DBTH.getType("_ZTS11ChildsChild");
  ASSERT_NE(nullptr, ChildsChildType);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  EXPECT_EQ(VTableForBase->getFunction(0), nullptr);

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  EXPECT_EQ(VTableForChild->getFunction(0), nullptr);

  const auto &VTableForChildsChild = DBTH.getVFTable(ChildsChildType);
  ASSERT_NE(nullptr, VTableForChildsChild);
  ASSERT_NE(nullptr, VTableForChildsChild->getFunction(0));
  EXPECT_EQ(VTableForChildsChild->getFunction(0)->getName(),
            "_ZN11ChildsChild3fooEv");
}

TEST(DBTHTest, VTableConstruction_12_c) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_12_c_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &ChildsChildType = DBTH.getType("_ZTS11ChildsChild");
  ASSERT_NE(nullptr, ChildsChildType);

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");

  const auto &VTableForChildsChild = DBTH.getVFTable(ChildsChildType);
  ASSERT_NE(nullptr, VTableForChildsChild);
  EXPECT_EQ(VTableForChildsChild->getFunction(0), nullptr);
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

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base4testEv");
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

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base4testEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  EXPECT_EQ(VTableForChild->getFunction(0), nullptr);
}

TEST(DBTHTest, VTableConstruction_16) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_16_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &ChildOfChildType = DBTH.getType("_ZTS12ChildOfChild");
  ASSERT_NE(nullptr, ChildOfChildType);
  const auto &BaseTwoType = DBTH.getType("_ZTS7BaseTwo");
  ASSERT_NE(nullptr, BaseTwoType);
  const auto &ChildTwoType = DBTH.getType("_ZTS8ChildTwo");
  ASSERT_NE(nullptr, ChildTwoType);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base3fooEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");

  const auto &VTableForChildOfChild = DBTH.getVFTable(ChildOfChildType);
  ASSERT_NE(nullptr, VTableForChildOfChild);
  EXPECT_EQ(VTableForChildOfChild->getFunction(0), nullptr);

  const auto &VTableForBaseTwo = DBTH.getVFTable(BaseTwoType);
  ASSERT_NE(nullptr, VTableForBaseTwo);
  ASSERT_NE(nullptr, VTableForBaseTwo->getFunction(0));
  EXPECT_EQ(VTableForBaseTwo->getFunction(0)->getName(),
            "_ZN7BaseTwo6foobarEv");

  const auto &VTableForChildTwo = DBTH.getVFTable(ChildTwoType);
  ASSERT_NE(nullptr, VTableForChildTwo);
  ASSERT_NE(nullptr, VTableForChildTwo->getFunction(0));
  EXPECT_EQ(VTableForChildTwo->getFunction(0)->getName(),
            "_ZN8ChildTwo6foobarEv");
}

TEST(DBTHTest, VTableConstruction_17) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_17_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  // Since Child2 is never used, it is sometimes optimized out by the compiler
  // const auto &Child2Type = DBTH.getType("Child2");
  // ASSERT_NE(nullptr, Child2Type);
  const auto &Base2Type = DBTH.getType("_ZTS5Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &KidType = DBTH.getType("_ZTS3Kid");
  ASSERT_NE(nullptr, KidType);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  EXPECT_EQ(VTableForBase->getFunction(0), nullptr);
  ASSERT_NE(nullptr, VTableForBase->getFunction(1));
  EXPECT_EQ(VTableForBase->getFunction(1)->getName(), "_ZN4Base3barEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_EQ(VTableForChild->getFunction(1), nullptr);
  ASSERT_NE(nullptr, VTableForChild->getFunction(2));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
  EXPECT_EQ(VTableForChild->getFunction(2)->getName(), "_ZN5Child3bazEv");

  const auto &VTableForBase2 = DBTH.getVFTable(Base2Type);
  ASSERT_NE(nullptr, VTableForBase2);
  EXPECT_EQ(VTableForBase2->getFunction(0), nullptr);
  ASSERT_NE(nullptr, VTableForBase2->getFunction(1));
  EXPECT_EQ(VTableForBase2->getFunction(2), nullptr);
  ASSERT_NE(nullptr, VTableForBase2->getFunction(3));
  EXPECT_EQ(VTableForBase2->getFunction(1)->getName(), "_ZN5Base23barEv");
  EXPECT_EQ(VTableForBase2->getFunction(3)->getName(), "_ZN5Base26foobarEv");

  const auto &VTableForKid = DBTH.getVFTable(KidType);
  ASSERT_NE(nullptr, VTableForKid);
  ASSERT_NE(nullptr, VTableForKid->getFunction(0));
  EXPECT_EQ(VTableForKid->getFunction(1), nullptr);
  ASSERT_NE(nullptr, VTableForKid->getFunction(2));
  EXPECT_EQ(VTableForKid->getFunction(3), nullptr);
  ASSERT_NE(nullptr, VTableForKid->getFunction(4));
  EXPECT_EQ(VTableForKid->getFunction(0)->getName(), "_ZN3Kid3fooEv");
  EXPECT_EQ(VTableForKid->getFunction(2)->getName(), "_ZN3Kid6barfooEv");
  EXPECT_EQ(VTableForKid->getFunction(4)->getName(), "_ZN3Kid3bauEv");
}

TEST(DBTHTest, VTableConstruction_18) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_18_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &Child2Type = DBTH.getType("_ZTS7Child_2");
  ASSERT_NE(nullptr, Child2Type);
  const auto &Child3Type = DBTH.getType("_ZTS7Child_3");
  ASSERT_NE(nullptr, Child3Type);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  EXPECT_EQ(VTableForBase->getFunction(0), nullptr);
  ASSERT_NE(nullptr, VTableForBase->getFunction(1));
  EXPECT_EQ(VTableForBase->getFunction(1)->getName(), "_ZN4Base3barEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");

  const auto &VTableForChild2 = DBTH.getVFTable(Child2Type);
  ASSERT_NE(nullptr, VTableForChild2);
  EXPECT_EQ(VTableForChild2->getFunction(0), nullptr);
  EXPECT_EQ(VTableForChild2->getFunction(1), nullptr);
  // Test
  // ASSERT_NE(nullptr, VTableForBase->getFunction(2));
  EXPECT_EQ(VTableForChild2->getFunction(2)->getName(), "_ZN7Child_26foobarEv");

  const auto &VTableForChild3 = DBTH.getVFTable(Child3Type);
  ASSERT_NE(nullptr, VTableForChild3);
  EXPECT_EQ(VTableForChild3->getFunction(0), nullptr);
  EXPECT_EQ(VTableForChild3->getFunction(1), nullptr);
  EXPECT_EQ(VTableForChild3->getFunction(2), nullptr);
  ASSERT_NE(nullptr, VTableForChild3->getFunction(3));
  EXPECT_EQ(VTableForChild3->getFunction(3)->getName(), "_ZN7Child_36barfooEv");
}

TEST(DBTHTest, VTableConstruction_19) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_19_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &FooType = DBTH.getType("_ZTS3Foo");
  ASSERT_NE(nullptr, FooType);
  const auto &BarType = DBTH.getType("_ZTS3Bar");
  ASSERT_NE(nullptr, BarType);
  const auto &LoremType = DBTH.getType("_ZTS5Lorem");
  ASSERT_NE(nullptr, LoremType);
  const auto &ImpsumType = DBTH.getType("_ZTS6Impsum");
  ASSERT_NE(nullptr, ImpsumType);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  ASSERT_NE(nullptr, VTableForBase->getFunction(0));
  EXPECT_EQ(VTableForBase->getFunction(0)->getName(), "_ZN4Base10publicbaseEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(),
            "_ZN5Child10publicbaseEv");

  const auto &VTableForBar = DBTH.getVFTable(BarType);
  ASSERT_NE(nullptr, VTableForBar);
  ASSERT_NE(nullptr, VTableForBar->getFunction(0));
  EXPECT_EQ(VTableForBar->getFunction(0)->getName(), "_ZN3Bar12protectedfooEv");

  const auto &VTableForFoo = DBTH.getVFTable(FooType);
  ASSERT_NE(nullptr, VTableForFoo);
  ASSERT_NE(nullptr, VTableForFoo->getFunction(0));
  EXPECT_EQ(VTableForFoo->getFunction(0)->getName(), "_ZN3Foo12protectedfooEv");

  const auto &VTableForLorem = DBTH.getVFTable(LoremType);
  ASSERT_NE(nullptr, VTableForLorem);
  ASSERT_NE(nullptr, VTableForLorem->getFunction(0));
  EXPECT_EQ(VTableForLorem->getFunction(0)->getName(),
            "_ZN5Lorem12privateloremEv");

  const auto &VTableForImpsum = DBTH.getVFTable(ImpsumType);
  ASSERT_NE(nullptr, VTableForImpsum);
  ASSERT_NE(nullptr, VTableForImpsum->getFunction(0));
  EXPECT_EQ(VTableForImpsum->getFunction(0)->getName(),
            "_ZN6Impsum12privateloremEv");
}

TEST(DBTHTest, VTableConstruction_20) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_20_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &Base2Type = DBTH.getType("_ZTS5Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  EXPECT_EQ(VTableForBase->getFunction(0), nullptr);
  ASSERT_NE(nullptr, VTableForBase->getFunction(1));
  EXPECT_EQ(VTableForBase->getFunction(1)->getName(), "_ZN4Base3barEv");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  ASSERT_NE(nullptr, VTableForChild->getFunction(0));
  EXPECT_EQ(VTableForChild->getFunction(1), nullptr);
  ASSERT_NE(nullptr, VTableForChild->getFunction(2));
  EXPECT_EQ(VTableForChild->getFunction(0)->getName(), "_ZN5Child3fooEv");
  EXPECT_EQ(VTableForChild->getFunction(2)->getName(), "_ZN5Child4bar2Ev");

  const auto &VTableForBase2 = DBTH.getVFTable(Base2Type);
  ASSERT_NE(nullptr, VTableForBase2);
  ASSERT_NE(nullptr, VTableForBase2->getFunction(0));
  EXPECT_EQ(VTableForBase2->getFunction(0)->getName(), "_ZN5Base24foo2Ev");
}

TEST(DBTHTest, VTableConstruction_21) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_21_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &Base2Type = DBTH.getType("_ZTS5Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &Base3Type = DBTH.getType("_ZTS5Base3");
  ASSERT_NE(nullptr, Base3Type);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &Child2Type = DBTH.getType("_ZTS6Child2");
  ASSERT_NE(nullptr, Child2Type);

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase);
  EXPECT_EQ(VTableForBase->getFunction(0), nullptr);
  EXPECT_EQ(VTableForBase->getFunction(1), nullptr);
  EXPECT_EQ(VTableForBase->getFunction(2), nullptr);
  // Test
  // ASSERT_NE(nullptr, VTableForBase->getFunction(3));
  EXPECT_EQ(VTableForBase->getFunction(3)->getName(), "_ZN4Base3barEv");

  const auto &VTableForBase2 = DBTH.getVFTable(Base2Type);
  ASSERT_NE(nullptr, VTableForBase2);
  EXPECT_EQ(VTableForBase2->getFunction(0), nullptr);
  EXPECT_EQ(VTableForBase2->getFunction(1), nullptr);
  ASSERT_NE(nullptr, VTableForBase2->getFunction(2));
  EXPECT_EQ(VTableForBase2->getFunction(2)->getName(), "_ZN5Base24foo2Ev");

  const auto &VTableForBase3 = DBTH.getVFTable(Base3Type);
  ASSERT_NE(nullptr, VTableForBase3);
  EXPECT_EQ(nullptr, VTableForBase->getFunction(0));

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  EXPECT_EQ(VTableForChild->getFunction(0), nullptr);
  EXPECT_EQ(VTableForChild->getFunction(1), nullptr);
  ASSERT_NE(nullptr, VTableForChild->getFunction(2));
  EXPECT_EQ(VTableForChild->getFunction(2)->getName(), "_ZN5Child3fooEv");
  EXPECT_EQ(VTableForChild->getFunction(3), nullptr);
  ASSERT_NE(nullptr, VTableForChild->getFunction(4));
  EXPECT_EQ(VTableForChild->getFunction(4)->getName(), "_ZN5Child4bar2Ev");

  const auto &VTableForChild2 = DBTH.getVFTable(Child2Type);
  ASSERT_NE(nullptr, VTableForChild2);
  EXPECT_EQ(VTableForChild2->getFunction(0), nullptr);
  EXPECT_EQ(VTableForChild2->getFunction(1), nullptr);
  EXPECT_EQ(VTableForChild2->getFunction(2), nullptr);
  EXPECT_EQ(VTableForChild2->getFunction(3), nullptr);
  EXPECT_EQ(VTableForChild2->getFunction(4), nullptr);
  ASSERT_NE(nullptr, VTableForChild2->getFunction(5));
  EXPECT_EQ(VTableForChild2->getFunction(5)->getName(), "_ZN6Child26foobarEv");
}

/*
--------------------------------
TransitivelyReachableTypes Tests
--------------------------------
*/

TEST(DBTHTest, TransitivelyReachableTypes_1) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_1_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_2) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_2_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_3) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_3_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_4) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_4_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_5) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_5_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &OtherBaseType = DBTH.getType("_ZTS9OtherBase");
  ASSERT_NE(nullptr, OtherBaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesOtherBase = DBTH.getSubTypes(OtherBaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesOtherBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_TRUE(ReachableTypesOtherBase.count(OtherBaseType));
  EXPECT_TRUE(ReachableTypesOtherBase.count(ChildType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_FALSE(ReachableTypesChild.count(OtherBaseType));
}

TEST(DBTHTest, TransitivelyReachableTypes_6) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_6_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_7) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_7_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &AType = DBTH.getType("_ZTS1A");
  ASSERT_NE(nullptr, AType);
  const auto &BType = DBTH.getType("_ZTS1B");
  ASSERT_NE(nullptr, BType);
  const auto &CType = DBTH.getType("_ZTS1C");
  ASSERT_NE(nullptr, CType);
  const auto &DType = DBTH.getType("_ZTS1D");
  ASSERT_NE(nullptr, DType);
  const auto &XType = DBTH.getType("_ZTS1X");
  ASSERT_NE(nullptr, XType);
  const auto &YType = DBTH.getType("_ZTS1Y");
  ASSERT_NE(nullptr, YType);
  const auto &ZType = DBTH.getType("_ZTS1Z");
  ASSERT_NE(nullptr, ZType);

  auto ReachableTypesA = DBTH.getSubTypes(AType);
  auto ReachableTypesB = DBTH.getSubTypes(BType);
  auto ReachableTypesC = DBTH.getSubTypes(CType);
  auto ReachableTypesD = DBTH.getSubTypes(DType);
  auto ReachableTypesX = DBTH.getSubTypes(XType);
  auto ReachableTypesY = DBTH.getSubTypes(YType);
  auto ReachableTypesZ = DBTH.getSubTypes(ZType);

  EXPECT_EQ(ReachableTypesA.size(), 5U);
  EXPECT_EQ(ReachableTypesB.size(), 2U);
  EXPECT_EQ(ReachableTypesC.size(), 2U);
  EXPECT_EQ(ReachableTypesD.size(), 1U);
  EXPECT_EQ(ReachableTypesX.size(), 3U);
  EXPECT_EQ(ReachableTypesY.size(), 2U);
  EXPECT_EQ(ReachableTypesZ.size(), 1U);

  EXPECT_TRUE(ReachableTypesA.count(AType));
  EXPECT_TRUE(ReachableTypesA.count(BType));
  EXPECT_TRUE(ReachableTypesA.count(CType));
  EXPECT_TRUE(ReachableTypesA.count(DType));
  EXPECT_FALSE(ReachableTypesA.count(XType));
  EXPECT_FALSE(ReachableTypesA.count(YType));
  EXPECT_TRUE(ReachableTypesA.count(ZType));

  EXPECT_FALSE(ReachableTypesB.count(AType));
  EXPECT_TRUE(ReachableTypesB.count(BType));
  EXPECT_FALSE(ReachableTypesB.count(CType));
  EXPECT_TRUE(ReachableTypesB.count(DType));
  EXPECT_FALSE(ReachableTypesB.count(XType));
  EXPECT_FALSE(ReachableTypesB.count(YType));
  EXPECT_FALSE(ReachableTypesB.count(ZType));

  EXPECT_FALSE(ReachableTypesC.count(AType));
  EXPECT_FALSE(ReachableTypesC.count(BType));
  EXPECT_TRUE(ReachableTypesC.count(CType));
  EXPECT_FALSE(ReachableTypesC.count(DType));
  EXPECT_FALSE(ReachableTypesC.count(XType));
  EXPECT_FALSE(ReachableTypesC.count(YType));
  EXPECT_TRUE(ReachableTypesC.count(ZType));

  EXPECT_FALSE(ReachableTypesD.count(AType));
  EXPECT_FALSE(ReachableTypesD.count(BType));
  EXPECT_FALSE(ReachableTypesD.count(CType));
  EXPECT_TRUE(ReachableTypesD.count(DType));
  EXPECT_FALSE(ReachableTypesD.count(XType));
  EXPECT_FALSE(ReachableTypesD.count(YType));
  EXPECT_FALSE(ReachableTypesD.count(ZType));

  EXPECT_FALSE(ReachableTypesX.count(AType));
  EXPECT_FALSE(ReachableTypesX.count(BType));
  EXPECT_FALSE(ReachableTypesX.count(CType));
  EXPECT_FALSE(ReachableTypesX.count(DType));
  EXPECT_TRUE(ReachableTypesX.count(XType));
  EXPECT_TRUE(ReachableTypesX.count(YType));
  EXPECT_TRUE(ReachableTypesX.count(ZType));

  EXPECT_FALSE(ReachableTypesY.count(AType));
  EXPECT_FALSE(ReachableTypesY.count(BType));
  EXPECT_FALSE(ReachableTypesY.count(CType));
  EXPECT_FALSE(ReachableTypesY.count(DType));
  EXPECT_FALSE(ReachableTypesY.count(XType));
  EXPECT_TRUE(ReachableTypesY.count(YType));
  EXPECT_TRUE(ReachableTypesY.count(ZType));

  EXPECT_FALSE(ReachableTypesZ.count(AType));
  EXPECT_FALSE(ReachableTypesZ.count(BType));
  EXPECT_FALSE(ReachableTypesZ.count(CType));
  EXPECT_FALSE(ReachableTypesZ.count(DType));
  EXPECT_FALSE(ReachableTypesZ.count(XType));
  EXPECT_FALSE(ReachableTypesZ.count(YType));
  EXPECT_TRUE(ReachableTypesZ.count(ZType));
}

TEST(DBTHTest, TransitivelyReachableTypes_7_b) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_7_b_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &AType = DBTH.getType("A");
  ASSERT_NE(nullptr, AType);
  const auto &CType = DBTH.getType("_ZTS1C");
  ASSERT_NE(nullptr, CType);
  const auto &XType = DBTH.getType("X");
  ASSERT_NE(nullptr, XType);
  const auto &YType = DBTH.getType("_ZTS1Y");
  ASSERT_NE(nullptr, YType);
  const auto &ZType = DBTH.getType("_ZTS1Z");
  ASSERT_NE(nullptr, ZType);
  const auto &OmegaType = DBTH.getType("_ZTS5Omega");
  ASSERT_NE(nullptr, OmegaType);

  auto ReachableTypesA = DBTH.getSubTypes(AType);
  auto ReachableTypesC = DBTH.getSubTypes(CType);
  auto ReachableTypesX = DBTH.getSubTypes(XType);
  auto ReachableTypesY = DBTH.getSubTypes(YType);
  auto ReachableTypesZ = DBTH.getSubTypes(ZType);
  auto ReachableTypesOmega = DBTH.getSubTypes(OmegaType);

  EXPECT_EQ(ReachableTypesA.size(), 4U);
  EXPECT_EQ(ReachableTypesC.size(), 3U);
  EXPECT_EQ(ReachableTypesX.size(), 4U);
  EXPECT_EQ(ReachableTypesY.size(), 3U);
  EXPECT_EQ(ReachableTypesZ.size(), 2U);
  EXPECT_EQ(ReachableTypesOmega.size(), 1U);

  EXPECT_TRUE(ReachableTypesA.count(AType));
  EXPECT_TRUE(ReachableTypesA.count(CType));
  EXPECT_FALSE(ReachableTypesA.count(XType));
  EXPECT_FALSE(ReachableTypesA.count(YType));
  EXPECT_TRUE(ReachableTypesA.count(ZType));
  EXPECT_TRUE(ReachableTypesA.count(OmegaType));

  EXPECT_FALSE(ReachableTypesC.count(AType));
  EXPECT_TRUE(ReachableTypesC.count(CType));
  EXPECT_FALSE(ReachableTypesC.count(XType));
  EXPECT_FALSE(ReachableTypesC.count(YType));
  EXPECT_TRUE(ReachableTypesC.count(ZType));
  EXPECT_TRUE(ReachableTypesC.count(OmegaType));

  EXPECT_FALSE(ReachableTypesX.count(AType));
  EXPECT_FALSE(ReachableTypesX.count(CType));
  EXPECT_TRUE(ReachableTypesX.count(XType));
  EXPECT_TRUE(ReachableTypesX.count(YType));
  EXPECT_TRUE(ReachableTypesX.count(ZType));
  EXPECT_TRUE(ReachableTypesX.count(OmegaType));

  EXPECT_FALSE(ReachableTypesY.count(AType));
  EXPECT_FALSE(ReachableTypesY.count(CType));
  EXPECT_FALSE(ReachableTypesY.count(XType));
  EXPECT_TRUE(ReachableTypesY.count(YType));
  EXPECT_TRUE(ReachableTypesY.count(ZType));
  EXPECT_TRUE(ReachableTypesY.count(OmegaType));

  EXPECT_FALSE(ReachableTypesZ.count(AType));
  EXPECT_FALSE(ReachableTypesZ.count(CType));
  EXPECT_FALSE(ReachableTypesZ.count(XType));
  EXPECT_FALSE(ReachableTypesZ.count(YType));
  EXPECT_TRUE(ReachableTypesZ.count(ZType));
  EXPECT_TRUE(ReachableTypesZ.count(OmegaType));

  EXPECT_FALSE(ReachableTypesOmega.count(AType));
  EXPECT_FALSE(ReachableTypesOmega.count(CType));
  EXPECT_FALSE(ReachableTypesOmega.count(XType));
  EXPECT_FALSE(ReachableTypesOmega.count(YType));
  EXPECT_FALSE(ReachableTypesOmega.count(ZType));
  EXPECT_TRUE(ReachableTypesOmega.count(OmegaType));
}

TEST(DBTHTest, TransitivelyReachableTypes_8) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_8_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &NonvirtualClassType = DBTH.getType("_ZTS15NonvirtualClass");
  ASSERT_NE(nullptr, NonvirtualClassType);
  const auto &NonvirtualStructType = DBTH.getType("_ZTS16NonvirtualStruct");
  ASSERT_NE(nullptr, NonvirtualStructType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);
  auto ReachableTypesNonvirtualClass = DBTH.getSubTypes(NonvirtualClassType);
  auto ReachableTypesNonvirtualStruct = DBTH.getSubTypes(NonvirtualStructType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_EQ(ReachableTypesNonvirtualClass.size(), 1U);
  EXPECT_EQ(ReachableTypesNonvirtualStruct.size(), 1U);

  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_FALSE(ReachableTypesBase.count(NonvirtualClassType));
  EXPECT_FALSE(ReachableTypesBase.count(NonvirtualStructType));

  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
  EXPECT_FALSE(ReachableTypesChild.count(NonvirtualClassType));
  EXPECT_FALSE(ReachableTypesChild.count(NonvirtualStructType));

  EXPECT_FALSE(ReachableTypesNonvirtualClass.count(BaseType));
  EXPECT_FALSE(ReachableTypesNonvirtualClass.count(ChildType));
  EXPECT_TRUE(ReachableTypesNonvirtualClass.count(NonvirtualClassType));
  EXPECT_FALSE(ReachableTypesNonvirtualClass.count(NonvirtualStructType));

  EXPECT_FALSE(ReachableTypesNonvirtualStruct.count(BaseType));
  EXPECT_FALSE(ReachableTypesNonvirtualStruct.count(ChildType));
  EXPECT_FALSE(ReachableTypesNonvirtualStruct.count(NonvirtualClassType));
  EXPECT_TRUE(ReachableTypesNonvirtualStruct.count(NonvirtualStructType));
}

TEST(DBTHTest, TransitivelyReachableTypes_9) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_9_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_10) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_10_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_11) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_11_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_12) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_12_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_12_b) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_12_b_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &ChildsChildType = DBTH.getType("_ZTS11ChildsChild");
  ASSERT_NE(nullptr, ChildsChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);
  auto ReachableTypesChildsChild = DBTH.getSubTypes(ChildsChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 3U);
  EXPECT_EQ(ReachableTypesChild.size(), 2U);
  EXPECT_EQ(ReachableTypesChildsChild.size(), 1U);
  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildsChildType));
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildsChildType));
  EXPECT_FALSE(ReachableTypesChildsChild.count(BaseType));
  EXPECT_FALSE(ReachableTypesChildsChild.count(ChildType));
  EXPECT_TRUE(ReachableTypesChildsChild.count(ChildsChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_12_c) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_12_c_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &ChildsChildType = DBTH.getType("_ZTS11ChildsChild");
  ASSERT_NE(nullptr, ChildsChildType);

  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);
  auto ReachableTypesChildsChild = DBTH.getSubTypes(ChildsChildType);

  EXPECT_EQ(ReachableTypesChild.size(), 2U);
  EXPECT_EQ(ReachableTypesChildsChild.size(), 1U);

  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildsChildType));
  EXPECT_TRUE(ReachableTypesChildsChild.count(ChildsChildType));
}

/*
TEST(DBTHTest, BasicTHReconstruction_13) {
  Test file 13 has no types
}
*/

TEST(DBTHTest, TransitivelyReachableTypes_14) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_14_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);

  EXPECT_EQ(ReachableTypesBase.size(), 1U);
  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
}

TEST(DBTHTest, TransitivelyReachableTypes_15) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_15_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);

  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_FALSE(ReachableTypesChild.count(BaseType));
  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_16) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_16_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);
  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  // const auto &ChildsChildType = DBTH.getType("ChildsChild");
  //  Since ChildsChild is never used, it is optimized out
  //  ASSERT_EQ(nullptr, ChildsChildType);
  const auto &BaseTwoType = DBTH.getType("_ZTS7BaseTwo");
  ASSERT_NE(nullptr, BaseTwoType);
  const auto &ChildTwoType = DBTH.getType("_ZTS8ChildTwo");
  ASSERT_NE(nullptr, ChildTwoType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);
  // Since ChildsChild is never used, it is optimized out
  // auto ReachableTypesChildsChild = DBTH.getSubTypes(ChildsChildType);
  auto ReachableTypesBaseTwo = DBTH.getSubTypes(BaseTwoType);
  auto ReachableTypesChildTwo = DBTH.getSubTypes(ChildTwoType);

  EXPECT_EQ(ReachableTypesBase.size(), 3U);
  EXPECT_EQ(ReachableTypesChild.size(), 2U);
  // EXPECT_EQ(ReachableTypesChildsChild.size(), 1U);
  EXPECT_EQ(ReachableTypesBaseTwo.size(), 2U);
  EXPECT_EQ(ReachableTypesChildTwo.size(), 1U);

  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));

  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
  // EXPECT_TRUE(ReachableTypesChild.count(ChildsChildType));

  // EXPECT_TRUE(ReachableTypesChildsChild.count(ChildsChildType));

  EXPECT_TRUE(ReachableTypesBaseTwo.count(BaseTwoType));
  EXPECT_TRUE(ReachableTypesBaseTwo.count(ChildTwoType));

  EXPECT_TRUE(ReachableTypesChildTwo.count(ChildTwoType));
}

TEST(DBTHTest, TransitivelyReachableTypes_17) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_17_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  // const auto &Child2Type = DBTH.getType("Child2");
  // Since Child2 is never used, it is optimized out
  // ASSERT_EQ(nullptr, Child2Type);
  const auto &Base2Type = DBTH.getType("_ZTS5Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &KidType = DBTH.getType("_ZTS3Kid");
  ASSERT_NE(nullptr, KidType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);
  // Since Child2 is never used, it is optimized out
  // auto ReachableTypesChild2 = DBTH.getSubTypes(Child2Type);
  auto ReachableTypesBase2 = DBTH.getSubTypes(Base2Type);
  auto ReachableTypesKid = DBTH.getSubTypes(KidType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  // EXPECT_EQ(ReachableTypesChild2.size(), 1U);
  EXPECT_EQ(ReachableTypesBase2.size(), 2U);
  EXPECT_EQ(ReachableTypesKid.size(), 1U);

  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));

  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
  // EXPECT_TRUE(ReachableTypesChild.count(Child2Type));

  // EXPECT_TRUE(ReachableTypesChild2.count(Child2Type));

  EXPECT_TRUE(ReachableTypesBase2.count(Base2Type));
  // EXPECT_TRUE(ReachableTypesBase2.count(Child2Type));

  EXPECT_TRUE(ReachableTypesKid.count(KidType));
}

TEST(DBTHTest, TransitivelyReachableTypes_18) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_18_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &Child2Type = DBTH.getType("_ZTS7Child_2");
  ASSERT_NE(nullptr, Child2Type);
  const auto &Child3Type = DBTH.getType("_ZTS7Child_3");
  ASSERT_NE(nullptr, Child3Type);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);
  auto ReachableTypesChild2 = DBTH.getSubTypes(Child2Type);
  auto ReachableTypesChild3 = DBTH.getSubTypes(Child3Type);

  EXPECT_EQ(ReachableTypesBase.size(), 4U);
  EXPECT_EQ(ReachableTypesChild.size(), 3U);
  EXPECT_EQ(ReachableTypesChild2.size(), 2U);
  EXPECT_EQ(ReachableTypesChild3.size(), 1U);

  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));
  EXPECT_TRUE(ReachableTypesBase.count(Child2Type));
  EXPECT_TRUE(ReachableTypesBase.count(Child3Type));

  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
  EXPECT_TRUE(ReachableTypesChild.count(Child2Type));
  EXPECT_TRUE(ReachableTypesChild.count(Child3Type));

  EXPECT_TRUE(ReachableTypesChild2.count(Child2Type));
  EXPECT_TRUE(ReachableTypesChild3.count(Child3Type));
}

TEST(DBTHTest, TransitivelyReachableTypes_19) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_19_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &FooType = DBTH.getType("_ZTS3Foo");
  ASSERT_NE(nullptr, FooType);
  const auto &BarType = DBTH.getType("_ZTS3Bar");
  ASSERT_NE(nullptr, BarType);
  const auto &LoremType = DBTH.getType("_ZTS5Lorem");
  ASSERT_NE(nullptr, LoremType);
  const auto &ImpsumType = DBTH.getType("_ZTS6Impsum");
  ASSERT_NE(nullptr, ImpsumType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);
  auto ReachableTypesFoo = DBTH.getSubTypes(FooType);
  auto ReachableTypesBar = DBTH.getSubTypes(BarType);
  auto ReachableTypesLorem = DBTH.getSubTypes(LoremType);
  auto ReachableTypesImpsum = DBTH.getSubTypes(ImpsumType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);
  EXPECT_EQ(ReachableTypesFoo.size(), 2U);
  EXPECT_EQ(ReachableTypesBar.size(), 1U);
  EXPECT_EQ(ReachableTypesLorem.size(), 2U);
  EXPECT_EQ(ReachableTypesImpsum.size(), 1U);

  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));

  EXPECT_TRUE(ReachableTypesChild.count(ChildType));

  EXPECT_TRUE(ReachableTypesFoo.count(FooType));
  EXPECT_TRUE(ReachableTypesFoo.count(BarType));

  EXPECT_TRUE(ReachableTypesBar.count(BarType));

  EXPECT_TRUE(ReachableTypesLorem.count(LoremType));
  EXPECT_TRUE(ReachableTypesLorem.count(ImpsumType));

  EXPECT_TRUE(ReachableTypesImpsum.count(ImpsumType));
}

TEST(DBTHTest, TransitivelyReachableTypes_20) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_20_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &Base2Type = DBTH.getType("_ZTS5Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesBase2 = DBTH.getSubTypes(Base2Type);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);

  EXPECT_EQ(ReachableTypesBase.size(), 2U);
  EXPECT_EQ(ReachableTypesBase2.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 1U);

  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));

  EXPECT_TRUE(ReachableTypesBase2.count(Base2Type));
  EXPECT_TRUE(ReachableTypesBase2.count(ChildType));

  EXPECT_TRUE(ReachableTypesChild.count(ChildType));
}

TEST(DBTHTest, TransitivelyReachableTypes_21) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_21_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);

  // check for all types
  const auto &BaseType = DBTH.getType("_ZTS4Base");
  ASSERT_NE(nullptr, BaseType);
  const auto &Base2Type = DBTH.getType("_ZTS5Base2");
  ASSERT_NE(nullptr, Base2Type);
  const auto &Base3Type = DBTH.getType("_ZTS5Base3");
  ASSERT_NE(nullptr, Base3Type);
  const auto &ChildType = DBTH.getType("_ZTS5Child");
  ASSERT_NE(nullptr, ChildType);
  const auto &Child2Type = DBTH.getType("_ZTS6Child2");
  ASSERT_NE(nullptr, Child2Type);

  auto ReachableTypesBase = DBTH.getSubTypes(BaseType);
  auto ReachableTypesBase2 = DBTH.getSubTypes(Base2Type);
  auto ReachableTypesBase3 = DBTH.getSubTypes(Base3Type);
  auto ReachableTypesChild = DBTH.getSubTypes(ChildType);
  auto ReachableTypesChild2 = DBTH.getSubTypes(Child2Type);

  EXPECT_EQ(ReachableTypesBase.size(), 3U);
  EXPECT_EQ(ReachableTypesBase2.size(), 3U);
  EXPECT_EQ(ReachableTypesBase3.size(), 2U);
  EXPECT_EQ(ReachableTypesChild.size(), 2U);
  EXPECT_EQ(ReachableTypesChild2.size(), 1U);

  EXPECT_TRUE(ReachableTypesBase.count(BaseType));
  EXPECT_TRUE(ReachableTypesBase.count(ChildType));

  EXPECT_TRUE(ReachableTypesBase2.count(Base2Type));
  EXPECT_TRUE(ReachableTypesBase2.count(ChildType));

  EXPECT_TRUE(ReachableTypesBase3.count(Base3Type));
  EXPECT_TRUE(ReachableTypesBase3.count(Child2Type));

  EXPECT_TRUE(ReachableTypesChild.count(ChildType));

  EXPECT_TRUE(ReachableTypesChild2.count(Child2Type));
}

} // namespace psr

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  auto Res = RUN_ALL_TESTS();
  return Res;
}
