
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
}

} // namespace psr

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  auto Res = RUN_ALL_TESTS();
  return Res;
}
