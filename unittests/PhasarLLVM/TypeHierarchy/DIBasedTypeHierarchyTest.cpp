
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

namespace psr {

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

  // Since Child2 hasn't been created, it shouldn't exist and also not be found
  // via DBTH.getType("Child2")
  const auto &Child2Type = DBTH.getType("Child2");
  EXPECT_FALSE(Child2Type);

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
                        "type_hierarchies/type_hierarchy_18_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  EXPECT_TRUE(DBTH.hasType(BaseType));

  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild);
  const auto &VTableForChildFunction0 = VTableForChild->getFunction(0);
  ASSERT_NE(nullptr, VTableForChildFunction0);
  EXPECT_TRUE(VTableForChildFunction0->getName() == "_ZN5Child3fooEv");

  const auto &Child2Type = DBTH.getType("Child_2");
  ASSERT_NE(nullptr, Child2Type);
  ASSERT_TRUE(DBTH.hasVFTable(Child2Type));

  const auto &VTableForChild2 = DBTH.getVFTable(Child2Type);
  ASSERT_NE(nullptr, VTableForChild2);
  const auto &VTableForChild2Function2 = VTableForChild2->getFunction(2);
  ASSERT_NE(nullptr, VTableForChild2Function2);

  EXPECT_TRUE(VTableForChild2Function2->getName() == "_ZN7Child_26foobarEv");

  const auto &Child3Type = DBTH.getType("Child_3");
  ASSERT_NE(nullptr, Child3Type);

  EXPECT_TRUE(DBTH.hasType(Child3Type));
  EXPECT_TRUE(DBTH.hasVFTable(Child3Type));

  // subtypes
  const auto &BaseSubTypes = DBTH.getSubTypes(BaseType);
  ASSERT_TRUE(!BaseSubTypes.empty());
  EXPECT_TRUE(BaseSubTypes.find(ChildType) != BaseSubTypes.end());
  EXPECT_TRUE(BaseSubTypes.find(Child2Type) != BaseSubTypes.end());
  EXPECT_TRUE(BaseSubTypes.find(Child3Type) != BaseSubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_4) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_20_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &BaseType = DBTH.getType("Base");
  ASSERT_NE(nullptr, BaseType);
  EXPECT_TRUE(DBTH.hasType(BaseType));

  const auto &Base2Type = DBTH.getType("Base2");
  ASSERT_NE(nullptr, Base2Type);
  EXPECT_TRUE(DBTH.hasType(Base2Type));

  const auto &ChildType = DBTH.getType("Child");
  ASSERT_NE(nullptr, ChildType);
  EXPECT_TRUE(DBTH.hasType(ChildType));

  const auto &BaseSubTypes = DBTH.getSubTypes(BaseType);
  const auto &Base2SubTypes = DBTH.getSubTypes(Base2Type);

  EXPECT_TRUE(BaseSubTypes.find(ChildType) != BaseSubTypes.end());
  EXPECT_TRUE(Base2SubTypes.find(ChildType) != Base2SubTypes.end());
  EXPECT_TRUE(DBTH.hasVFTable(ChildType));
}

TEST(DBTHTest, BasicTHReconstruction_5) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_21_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

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

  const auto &BaseSubTypes = DBTH.getSubTypes(BaseType);
  const auto &Base2SubTypes = DBTH.getSubTypes(Base2Type);

  EXPECT_TRUE(BaseSubTypes.find(ChildType) != BaseSubTypes.end());
  EXPECT_TRUE(Base2SubTypes.find(Child2Type) != Base2SubTypes.end());
}

TEST(DBTHTest, BasicTHReconstruction_6) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_21_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

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

  ASSERT_TRUE(DBTH.hasVFTable(BaseType));
  ASSERT_TRUE(DBTH.hasVFTable(Base2Type));
  EXPECT_FALSE(DBTH.hasVFTable(Base3Type));
  ASSERT_TRUE(DBTH.hasVFTable(ChildType));
  ASSERT_TRUE(DBTH.hasVFTable(Child2Type));

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  ASSERT_NE(nullptr, VTableForBase->getFunction(3));
  if (VTableForBase->getFunction(3)) {
    EXPECT_EQ(VTableForBase->getFunction(3)->getName(), "_ZN4Base3barEv");
  }

  const auto &VTableForBase2 = DBTH.getVFTable(Base2Type);
  ASSERT_NE(nullptr, VTableForBase2->getFunction(2));
  EXPECT_EQ(VTableForBase2->getFunction(2)->getName(), "_ZN5Base24foo2Ev");

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  ASSERT_NE(nullptr, VTableForChild->getFunction(2));
  EXPECT_EQ(VTableForChild->getFunction(2)->getName(), "_ZN5Child3fooEv");

  ASSERT_NE(nullptr, VTableForChild->getFunction(4));
  EXPECT_EQ(VTableForChild->getFunction(4)->getName(), "_ZN5Child4bar2Ev");

  const auto &VTableForChild2 = DBTH.getVFTable(Child2Type);
  ASSERT_NE(nullptr, VTableForChild2->getFunction(5));
  EXPECT_EQ(VTableForChild2->getFunction(5)->getName(), "_ZN6Child26foobarEv");
}

} // namespace psr

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  auto Res = RUN_ALL_TESTS();
  return Res;
}
