
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
  EXPECT_TRUE(BaseType);
  if (BaseType) {
    EXPECT_TRUE(DBTH.hasType(BaseType));
    EXPECT_TRUE(DBTH.hasVFTable(BaseType));
  }

  const auto &ChildType = DBTH.getType("Child");
  EXPECT_TRUE(ChildType);
  if (ChildType) {
    EXPECT_TRUE(DBTH.hasType(ChildType));
    EXPECT_TRUE(SubTypes.find(ChildType) != SubTypes.end());

    EXPECT_TRUE(DBTH.hasVFTable(ChildType));
    const auto &VTableForChild = DBTH.getVFTable(ChildType);
    EXPECT_TRUE(VTableForChild->getFunction(0)->getName() == "_ZN5Child3fooEv");
  }
}

TEST(DBTHTest, BasicTHReconstruction_2) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_17_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &BaseType = DBTH.getType("Base");
  EXPECT_TRUE(BaseType);
  if (BaseType) {
    EXPECT_TRUE(DBTH.hasType(BaseType));
    EXPECT_TRUE(DBTH.hasVFTable(BaseType));
  }

  const auto &ChildType = DBTH.getType("Child");
  const auto &BaseSubTypes = DBTH.getSubTypes(DBTH.getType("Base"));
  EXPECT_TRUE(ChildType);
  if (ChildType) {
    EXPECT_TRUE(DBTH.hasType(ChildType));
    EXPECT_TRUE(DBTH.hasVFTable(ChildType));
    EXPECT_TRUE(BaseSubTypes.count(ChildType));

    const auto &VTableForChild = DBTH.getVFTable(ChildType);
    EXPECT_TRUE(VTableForChild);
    if (VTableForChild) {
      EXPECT_TRUE(VTableForChild->getFunction(0)->getName() ==
                  "_ZN5Child3fooEv");
    }
  }

  const auto &Base2Type = DBTH.getType("Base2");
  EXPECT_TRUE(Base2Type);
  if (Base2Type) {
    EXPECT_TRUE(DBTH.hasType(Base2Type));
    EXPECT_TRUE(DBTH.hasVFTable(Base2Type));
  }

  // Since Child2 hasn't been created, it shouldn't exist and also not be found
  // via DBTH.getType("Child2")
  const auto &Child2Type = DBTH.getType("Child2");
  EXPECT_FALSE(Child2Type);

  const auto &KidType = DBTH.getType("Kid");
  EXPECT_TRUE(KidType);
  if (KidType) {
    EXPECT_TRUE(DBTH.hasType(KidType));
    EXPECT_TRUE(DBTH.hasVFTable(KidType));
    const auto &VTableForKid = DBTH.getVFTable(KidType);

    EXPECT_TRUE(VTableForKid);
    if (VTableForKid) {
      EXPECT_TRUE(VTableForKid->getFunction(0)->getName() == "_ZN3Kid3fooEv");
    }
  }

  const auto &VTableForBase2 = DBTH.getVFTable(Base2Type);
  EXPECT_TRUE(VTableForBase2);
  if (VTableForBase2) {
    EXPECT_TRUE(VTableForBase2->getFunction(1));
    if (VTableForBase2->getFunction(1)) {
      EXPECT_TRUE(VTableForBase2->getFunction(1)->getName() ==
                  "_ZN5Base23barEv");
    }
    EXPECT_TRUE(VTableForBase2->getFunction(3));
    if (VTableForBase2->getFunction(3)) {
      EXPECT_TRUE(VTableForBase2->getFunction(3)->getName() ==
                  "_ZN5Base26foobarEv");
    }
  }
}

TEST(DBTHTest, BasicTHReconstruction_3) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_18_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &BaseType = DBTH.getType("Base");
  EXPECT_TRUE(BaseType);
  if (BaseType) {
    EXPECT_TRUE(DBTH.hasType(BaseType));
  } else {
    // EXPECT_TRUE(BaseType) will make the unittest fail and since BaseType is
    // needed further down, the test cannot continue
    return;
  }

  const auto &ChildType = DBTH.getType("Child");
  EXPECT_TRUE(ChildType);
  if (ChildType) {
    EXPECT_TRUE(DBTH.hasType(ChildType));
    EXPECT_TRUE(DBTH.hasVFTable(ChildType));

    const auto &VTableForChild = DBTH.getVFTable(DBTH.getType("Child"));
    const auto &VTableForChildFunction0 = VTableForChild->getFunction(0);
    EXPECT_TRUE(VTableForChildFunction0);
    if (VTableForChildFunction0) {
      EXPECT_TRUE(VTableForChildFunction0->getName() == "_ZN5Child3fooEv");
    }
  }

  const auto &Child2Type = DBTH.getType("Child_2");
  EXPECT_TRUE(Child2Type);
  if (Child2Type) {
    EXPECT_TRUE(DBTH.hasType(Child2Type));
    EXPECT_TRUE(DBTH.hasVFTable(Child2Type));

    const auto &VTableForChild2 = DBTH.getVFTable(Child2Type);

    /*for (const auto &Curr : VTableForChild2->getAllFunctions()) {
      if (Curr) {
        llvm::outs() << "exists: " << Curr->getName() << "\n";
        llvm::outs().flush();
      } else {
        llvm::outs() << "nullptr\n";
        llvm::outs().flush();
      }
    }*/

    const auto &VTableForChild2Function2 = VTableForChild2->getFunction(2);
    EXPECT_TRUE(VTableForChild2Function2);

    if (VTableForChild2Function2) {
      EXPECT_TRUE(VTableForChild2Function2->getName() ==
                  "_ZN7Child_26foobarEv");
    }
  }

  const auto &Child3Type = DBTH.getType("Child_3");
  EXPECT_TRUE(Child3Type);
  if (Child3Type) {
    EXPECT_TRUE(DBTH.hasType(Child3Type));
    EXPECT_TRUE(DBTH.hasVFTable(Child3Type));
  }

  // subtypes
  const auto &BaseSubTypes = DBTH.getSubTypes(BaseType);
  EXPECT_TRUE(!BaseSubTypes.empty());
  if (!BaseSubTypes.empty()) {
    EXPECT_TRUE(BaseSubTypes.find(ChildType) != BaseSubTypes.end());
    EXPECT_TRUE(BaseSubTypes.find(Child2Type) != BaseSubTypes.end());
    EXPECT_TRUE(BaseSubTypes.find(Child3Type) != BaseSubTypes.end());
  }
}

TEST(DBTHTest, BasicTHReconstruction_4) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_20_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &BaseType = DBTH.getType("Base");
  EXPECT_TRUE(BaseType);
  if (BaseType) {
    EXPECT_TRUE(DBTH.hasType(BaseType));
  }

  const auto &Base2Type = DBTH.getType("Base2");
  EXPECT_TRUE(Base2Type);
  if (Base2Type) {
    EXPECT_TRUE(DBTH.hasType(Base2Type));
  }

  const auto &ChildType = DBTH.getType("Child");
  EXPECT_TRUE(ChildType);
  if (ChildType) {
    EXPECT_TRUE(DBTH.hasType(ChildType));
  }

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
  EXPECT_TRUE(BaseType);
  if (BaseType) {
    EXPECT_TRUE(DBTH.hasType(BaseType));
  }

  const auto &Base2Type = DBTH.getType("Base2");
  EXPECT_TRUE(Base2Type);
  if (Base2Type) {
    EXPECT_TRUE(DBTH.hasType(Base2Type));
  }

  const auto &Base3Type = DBTH.getType("Base3");
  EXPECT_TRUE(Base3Type);
  if (Base3Type) {
    EXPECT_TRUE(DBTH.hasType(Base3Type));
  }

  const auto &ChildType = DBTH.getType("Child");
  EXPECT_TRUE(ChildType);
  if (ChildType) {
    EXPECT_TRUE(DBTH.hasType(ChildType));
  }

  const auto &Child2Type = DBTH.getType("Child2");
  EXPECT_TRUE(Child2Type);
  if (Child2Type) {
    EXPECT_TRUE(DBTH.hasType(Child2Type));
  }

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
  EXPECT_TRUE(BaseType);
  if (BaseType) {
    EXPECT_TRUE(DBTH.hasType(BaseType));
  }

  const auto &Base2Type = DBTH.getType("Base2");
  EXPECT_TRUE(Base2Type);
  if (Base2Type) {
    EXPECT_TRUE(DBTH.hasType(Base2Type));
  }

  const auto &Base3Type = DBTH.getType("Base3");
  EXPECT_TRUE(Base3Type);
  if (Base3Type) {
    EXPECT_TRUE(DBTH.hasType(Base3Type));
  }

  const auto &ChildType = DBTH.getType("Child");
  EXPECT_TRUE(ChildType);
  if (ChildType) {
    EXPECT_TRUE(DBTH.hasType(ChildType));
  }

  const auto &Child2Type = DBTH.getType("Child2");
  EXPECT_TRUE(Child2Type);
  if (Child2Type) {
    EXPECT_TRUE(DBTH.hasType(Child2Type));
  }

  EXPECT_TRUE(DBTH.hasVFTable(BaseType));
  EXPECT_TRUE(DBTH.hasVFTable(Base2Type));
  EXPECT_FALSE(DBTH.hasVFTable(Base3Type));
  EXPECT_TRUE(DBTH.hasVFTable(ChildType));
  EXPECT_TRUE(DBTH.hasVFTable(Child2Type));

  // _ZN4Base3barEv
  // _ZN5Base24foo2Ev
  // _ZN5Child4bar2Ev
  // _ZN5Child3fooEv
  //

  const auto &VTableForBase = DBTH.getVFTable(BaseType);
  EXPECT_TRUE(VTableForBase->getFunction(3));
  if (VTableForBase->getFunction(3)) {
    EXPECT_EQ(VTableForBase->getFunction(3)->getName(), "_ZN4Base3barEv");
  }

  const auto &VTableForBase2 = DBTH.getVFTable(Base2Type);
  EXPECT_TRUE(VTableForBase2->getFunction(2));
  if (VTableForBase2->getFunction(2)) {
    EXPECT_EQ(VTableForBase2->getFunction(2)->getName(), "_ZN5Base24foo2Ev");
  }

  const auto &VTableForChild = DBTH.getVFTable(ChildType);
  EXPECT_TRUE(VTableForChild->getFunction(2));
  if (VTableForChild->getFunction(2)) {
    EXPECT_EQ(VTableForChild->getFunction(2)->getName(), "_ZN5Child3fooEv");
  }

  EXPECT_TRUE(VTableForChild->getFunction(4));
  if (VTableForChild->getFunction(4)) {
    EXPECT_EQ(VTableForChild->getFunction(4)->getName(), "_ZN5Child4bar2Ev");
  }

  const auto &VTableForChild2 = DBTH.getVFTable(Child2Type);
  EXPECT_TRUE(VTableForChild2->getFunction(5));
  if (VTableForChild2->getFunction(5)) {
    EXPECT_EQ(VTableForChild2->getFunction(5)->getName(),
              "_ZN6Child26foobarEv");
  }
}

} // namespace psr

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  auto Res = RUN_ALL_TESTS();
  return Res;
}
