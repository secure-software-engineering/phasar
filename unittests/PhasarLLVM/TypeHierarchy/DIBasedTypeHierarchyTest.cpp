
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/Demangle/Demangle.h"
#include "llvm/Support/ManagedStatic.h"

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

TEST(DBTHTest, BasicTHReconstruction_3) {
  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_7_cpp_dbg.ll"});
  DIBasedTypeHierarchy DBTH(IRDB);
}

TEST(DBTHTest, THConstructionException) {
  LLVMProjectIRDB IRDB(unittest::PathToLLTestFiles +
                       "type_hierarchies/type_hierarchy_15_cpp_dbg.ll");
  DIBasedTypeHierarchy DBTH(IRDB);
}

// check if the vtables are constructed correctly in more complex scenarios
TEST(DBTHTest, VTableConstruction) {
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

  // Creates an empty type hierarchy
  DIBasedTypeHierarchy TH1(IRDB1);
  DIBasedTypeHierarchy TH2(IRDB2);
  DIBasedTypeHierarchy TH3(IRDB3);
  DIBasedTypeHierarchy TH4(IRDB4);
  DIBasedTypeHierarchy TH5(IRDB5);
  DIBasedTypeHierarchy TH6(IRDB6);

  // ASSERT_TRUE(TH1.hasVFTable(TH1.getType("struct.Base")));
  // ASSERT_TRUE(TH1.hasVFTable(TH1.getType("struct.Child")));
}

TEST(DBTHTest, TransitivelyReachableTypes) {
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

  // auto ReachableTypesBase1 = TH1.getSubTypes(TH1.getType("struct.Base"));
  // auto ReachableTypesChild1 = TH1.getSubTypes(TH1.getType("struct.Child"));
  // auto ReachableTypesA2 = TH2.getSubTypes(TH2.getType("struct.A"));

  // Will be way less dangerous to have an interface (like a map) between the
  // llvm given name of class & struct (i.e. struct.Base.base ...) and the name
  // inside phasar (i.e. just Base) and never work with the llvm name inside
  // phasar

  // ASSERT_TRUE(ReachableTypesBase3.count(TH3.getType("struct.Base")));
}

// Failing test case
PHASAR_SKIP_TEST(TEST(DBTHTest, HandleSTLString) {
  // If we use libcxx this won't work since internal implementation is different
  LIBCPP_GTEST_SKIP;

  LLVMProjectIRDB IRDB({unittest::PathToLLTestFiles +
                        "type_hierarchies/type_hierarchy_13_cpp_dbg.ll"});
  DIBasedTypeHierarchy TH(IRDB);
  // NOTE: Even if using libstdc++, depending on the version the generated IR is
  // different; so, we cannot assert on the number of types here
  // EXPECT_EQ(TH.getAllTypes().size(), 7U);
  // EXPECT_TRUE(TH.hasType(TH.getType("class.std::__cxx11::basic_string")));
  // EXPECT_TRUE(TH.hasType(
})

} // namespace psr

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  auto Res = RUN_ALL_TESTS();
  return Res;
}
