
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "TestConfig.h"
#include "gtest/gtest.h"

#include <iomanip>

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
  EXPECT_TRUE(ReachableTypesA2.size() == 5U);
  EXPECT_TRUE(ReachableTypesB2.count(TH2.getType("B")));
  EXPECT_TRUE(ReachableTypesB2.count(TH2.getType("D")));
  EXPECT_TRUE(ReachableTypesB2.size() == 2U);
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
