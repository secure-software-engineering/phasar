
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

  EXPECT_TRUE(DBTH.hasType(DBTH.getType("Base")));
  EXPECT_TRUE(DBTH.hasType(DBTH.getType("Child")));
  EXPECT_TRUE(SubTypes.find(DBTH.getType("Child")) != SubTypes.end());

  EXPECT_FALSE(DBTH.hasVFTable(DBTH.getType("Base")));
  EXPECT_TRUE(DBTH.hasVFTable(DBTH.getType("Child")));

  const auto &VTableForChild = DBTH.getVFTable(DBTH.getType("Child"));
  EXPECT_TRUE(VTableForChild->getFunction(0)->getName() == "_ZN5Child3fooEv");
}

TEST(DBTHTest, BasicTHReconstruction_2) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_17_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &Types = DBTH.getAllTypes();
  const auto &BaseSubTypes = DBTH.getSubTypes(DBTH.getType("Base"));
  const auto &Base2SubTypes = DBTH.getSubTypes(DBTH.getType("Base"));

  EXPECT_TRUE(DBTH.hasType(DBTH.getType("Base")));
  EXPECT_TRUE(DBTH.hasType(DBTH.getType("Child")));
  EXPECT_TRUE(BaseSubTypes.find(DBTH.getType("Child")) != BaseSubTypes.end());
  // since there is no instance of Child2, there shouldn't be one in the type
  // hierarchy
  EXPECT_TRUE(BaseSubTypes.find(DBTH.getType("Child2")) == BaseSubTypes.end());

  EXPECT_TRUE(DBTH.hasType(DBTH.getType("Base2")));
  EXPECT_TRUE(DBTH.hasType(DBTH.getType("Kid")));

  EXPECT_TRUE(DBTH.hasVFTable(DBTH.getType("Child")));
  EXPECT_TRUE(DBTH.hasVFTable(DBTH.getType("Kid")));
  EXPECT_TRUE(DBTH.hasVFTable(DBTH.getType("Base2")));

  const auto &VTableForChild = DBTH.getVFTable(DBTH.getType("Child"));
  EXPECT_TRUE(VTableForChild->getFunction(0)->getName() == "_ZN5Child3fooEv");
  const auto &VTableForKid = DBTH.getVFTable(DBTH.getType("Kid"));
  EXPECT_TRUE(VTableForKid->getFunction(0)->getName() == "_ZN3Kid3fooEv");
  const auto &VTableForBase2 = DBTH.getVFTable(DBTH.getType("Base2"));
  EXPECT_TRUE(VTableForBase2->getFunction(0)->getName() == "_ZN5Base23barEv");
  EXPECT_TRUE(VTableForBase2->getFunction(1)->getName() ==
              "_ZN5Base26foobarEv");
}

TEST(DBTHTest, BasicTHReconstruction_3) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_18_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);

  const auto &Types = DBTH.getAllTypes();
  const auto &BaseSubTypes = DBTH.getSubTypes(DBTH.getType("Base"));
  const auto &Base2SubTypes = DBTH.getSubTypes(DBTH.getType("Base"));

  EXPECT_TRUE(DBTH.hasType(DBTH.getType("Base")));
  EXPECT_TRUE(DBTH.hasType(DBTH.getType("Child")));
  EXPECT_TRUE(DBTH.hasType(DBTH.getType("Child_2")));
  EXPECT_TRUE(DBTH.hasType(DBTH.getType("Child_3")));

  EXPECT_TRUE(BaseSubTypes.find(DBTH.getType("Child")) != BaseSubTypes.end());
  EXPECT_TRUE(BaseSubTypes.find(DBTH.getType("Child_2")) != BaseSubTypes.end());
  EXPECT_TRUE(BaseSubTypes.find(DBTH.getType("Child_3")) != BaseSubTypes.end());

  EXPECT_TRUE(DBTH.hasVFTable(DBTH.getType("Child")));
  EXPECT_TRUE(DBTH.hasVFTable(DBTH.getType("Child_2")));
  EXPECT_TRUE(DBTH.hasVFTable(DBTH.getType("Child_3")));

  const auto &VTableForChild = DBTH.getVFTable(DBTH.getType("Child"));
  EXPECT_TRUE(VTableForChild->getFunction(0)->getName() == "_ZN5Child3fooEv");
}

} // namespace psr

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  auto Res = RUN_ALL_TESTS();
  return Res;
}
