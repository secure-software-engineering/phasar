#include <iostream>

#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/isomorphism.hpp>

#include <gtest/gtest.h>

#include <phasar/Config/Configuration.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Utilities.h>

using namespace std;
using namespace psr;

namespace psr {
class LTHTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

// Check basic type hierarchy construction
TEST_F(LTHTest, BasicTHReconstruction_1) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_1_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 2);
  EXPECT_EQ(
      LTH.isSubType(LTH.getType("struct.Base"), LTH.getType("struct.Child")),
      true);
  EXPECT_EQ(
      LTH.isSuperType(LTH.getType("struct.Child"), LTH.getType("struct.Base")),
      true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Child")), true);
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Base"))->getFunction(0)->getName(),
      "_ZN4Base3fooEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(0)->getName(),
      "_ZN5Child3fooEv");
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 1);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 1);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST_F(LTHTest, BasicTHReconstruction_2) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_2_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 2);
  EXPECT_EQ(
      LTH.isSubType(LTH.getType("struct.Base"), LTH.getType("struct.Child")),
      true);
  EXPECT_EQ(
      LTH.isSuperType(LTH.getType("struct.Child"), LTH.getType("struct.Base")),
      true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Child")), true);
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Base"))->getFunction(0)->getName(),
      "_ZN4Base3fooEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(0)->getName(),
      "_ZN5Child3fooEv");
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 1);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 1);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST_F(LTHTest, BasicTHReconstruction_3) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_3_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 2);
  EXPECT_EQ(
      LTH.isSubType(LTH.getType("struct.Base"), LTH.getType("struct.Child")),
      true);
  EXPECT_EQ(
      LTH.isSuperType(LTH.getType("struct.Child"), LTH.getType("struct.Base")),
      true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Child")), true);
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Base"))->getFunction(0)->getName(),
      "_ZN4Base3fooEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Base"))->getFunction(1)->getName(),
      "_ZN4Base3barEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(0)->getName(),
      "_ZN5Child3fooEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(1)->getName(),
      "_ZN4Base3barEv");
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 2);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 2);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST_F(LTHTest, BasicTHReconstruction_4) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_4_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 2);
  EXPECT_EQ(
      LTH.isSubType(LTH.getType("struct.Base"), LTH.getType("struct.Child")),
      true);
  EXPECT_EQ(
      LTH.isSuperType(LTH.getType("struct.Child"), LTH.getType("struct.Base")),
      true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Child")), true);
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Base"))->getFunction(0)->getName(),
      "_ZN4Base3fooEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Base"))->getFunction(1)->getName(),
      "_ZN4Base3barEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(0)->getName(),
      "_ZN5Child3fooEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(1)->getName(),
      "_ZN4Base3barEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(2)->getName(),
      "_ZN5Child3tarEv");
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 2);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 3);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST_F(LTHTest, BasicTHReconstruction_5) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_5_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.OtherBase")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 3);
  EXPECT_EQ(
      LTH.isSubType(LTH.getType("struct.Base"), LTH.getType("struct.Child")),
      true);
  EXPECT_EQ(LTH.isSubType(LTH.getType("struct.OtherBase"),
                          LTH.getType("struct.Child")),
            true);
  EXPECT_EQ(
      LTH.isSuperType(LTH.getType("struct.Child"), LTH.getType("struct.Base")),
      true);
  EXPECT_EQ(LTH.isSuperType(LTH.getType("struct.Child"),
                            LTH.getType("struct.OtherBase")),
            true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.OtherBase")), true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Child")), true);
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Base"))->getFunction(0)->getName(),
      "_ZN4Base3fooEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Base"))->getFunction(1)->getName(),
      "_ZN4Base3barEv");
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.OtherBase"))
                ->getFunction(0)
                ->getName(),
            "_ZN9OtherBase3bazEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(0)->getName(),
      "_ZN5Child3fooEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(1)->getName(),
      "_ZN4Base3barEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(2)->getName(),
      "_ZN5Child3bazEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(3)->getName(),
      "_ZN5Child3tarEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(4)->getName(),
      "_ZThn8_N5Child3bazEv");
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 2);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.OtherBase"))->size(), 1);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 5);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.OtherBase")).size(), 2);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto OtherBaseReachable = LTH.getSubTypes(LTH.getType("struct.OtherBase"));
  EXPECT_EQ(OtherBaseReachable.count(LTH.getType("struct.OtherBase")), true);
  EXPECT_EQ(OtherBaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST_F(LTHTest, BasicTHReconstruction_6) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_12_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("class.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 2);
  EXPECT_EQ(
      LTH.isSubType(LTH.getType("class.Base"), LTH.getType("struct.Child")),
      true);
  EXPECT_EQ(
      LTH.isSuperType(LTH.getType("struct.Child"), LTH.getType("class.Base")),
      true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("class.Base")), true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Child")), true);
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("class.Base"))->getFunction(0)->getName(),
      "_ZN4Base3fooEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(0)->getName(),
      "_ZN5Child3fooEv");
  EXPECT_EQ(LTH.getVFTable(LTH.getType("class.Base"))->size(), 1);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 1);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("class.Base")).size(), 2);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("class.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("class.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST_F(LTHTest, BasicTHReconstruction_7) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_11_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  // has three types because of padding (introduction of intermediate type)
  EXPECT_EQ(LTH.getAllTypes().size(), 3);
  EXPECT_EQ(
      LTH.isSubType(LTH.getType("struct.Base"), LTH.getType("struct.Child")),
      true);
  EXPECT_EQ(
      LTH.isSuperType(LTH.getType("struct.Child"), LTH.getType("struct.Base")),
      true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasVFTable(LTH.getType("struct.Child")), true);
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Base"))->getFunction(0)->getName(),
      "_ZN4Base3fooEv");
  EXPECT_EQ(
      LTH.getVFTable(LTH.getType("struct.Child"))->getFunction(0)->getName(),
      "_ZN5Child3fooEv");
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 1);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 1);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

// check if the vtables are constructed correctly in more complex scenarios
TEST_F(LTHTest, VTableConstruction) {
  ProjectIRDB IRDB1(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_1_cpp.ll"});
  ProjectIRDB IRDB2(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_7_cpp.ll"});
  ProjectIRDB IRDB3(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_8_cpp.ll"});
  ProjectIRDB IRDB4(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_9_cpp.ll"});
  ProjectIRDB IRDB5(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_10_cpp.ll"});

  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH1(IRDB1);
  LLVMTypeHierarchy TH2(IRDB2);
  LLVMTypeHierarchy TH3(IRDB3);
  LLVMTypeHierarchy TH4(IRDB4);
  LLVMTypeHierarchy TH5(IRDB5);

  ASSERT_TRUE(TH1.hasVFTable(TH1.getType("struct.Base")));
  ASSERT_TRUE(TH1.hasVFTable(TH1.getType("struct.Child")));
  ASSERT_FALSE(TH1.hasVFTable(TH1.getType("struct.ANYTHING")));

  ASSERT_TRUE(TH2.hasVFTable(TH2.getType("struct.A")));
  ASSERT_TRUE(TH2.hasVFTable(TH2.getType("struct.B")));
  ASSERT_TRUE(TH2.hasVFTable(TH2.getType("struct.C")));
  ASSERT_TRUE(TH2.hasVFTable(TH2.getType("struct.D")));
  ASSERT_TRUE(TH2.hasVFTable(TH2.getType("struct.X")));
  ASSERT_TRUE(TH2.hasVFTable(TH2.getType("struct.Y")));
  ASSERT_TRUE(TH2.hasVFTable(TH2.getType("struct.Z")));

  ASSERT_TRUE(TH3.hasVFTable(TH3.getType("struct.Base")));
  ASSERT_TRUE(TH3.hasVFTable(TH3.getType("struct.Child")));
  ASSERT_FALSE(TH3.hasVFTable(TH3.getType("class.NonvirtualClass")));
  ASSERT_FALSE(TH3.hasVFTable(TH3.getType("struct.NonvirtualStruct")));

  ASSERT_TRUE(TH4.hasVFTable(TH4.getType("struct.Base")));
  ASSERT_TRUE(TH4.hasVFTable(TH4.getType("struct.Child")));

  ASSERT_TRUE(TH5.hasVFTable(TH5.getType("struct.Base")));
  ASSERT_TRUE(TH5.hasVFTable(TH5.getType("struct.Child")));

  ASSERT_TRUE(cxx_demangle(TH1.getVFTable(TH1.getType("struct.Base"))
                               ->getFunction(0)
                               ->getName()) == "Base::foo()");
  ASSERT_TRUE(TH1.getVFTable(TH1.getType("struct.Base"))->size() == 1);
  ASSERT_TRUE(cxx_demangle(TH1.getVFTable(TH1.getType("struct.Child"))
                               ->getFunction(0)
                               ->getName()) == "Child::foo()");
  ASSERT_TRUE(TH1.getVFTable(TH1.getType("struct.Child"))->size() == 1);

  ASSERT_TRUE(
      cxx_demangle(
          TH2.getVFTable(TH2.getType("struct.A"))->getFunction(0)->getName()) ==
      "A::f()");
  ASSERT_TRUE(TH2.getVFTable(TH2.getType("struct.A"))->size() == 1);
  ASSERT_TRUE(
      cxx_demangle(
          TH2.getVFTable(TH2.getType("struct.B"))->getFunction(0)->getName()) ==
      "A::f()");
  ASSERT_TRUE(TH2.getVFTable(TH2.getType("struct.B"))->size() == 1);
  ASSERT_TRUE(
      cxx_demangle(
          TH2.getVFTable(TH2.getType("struct.C"))->getFunction(0)->getName()) ==
      "A::f()");
  ASSERT_TRUE(

      TH2.getVFTable(TH2.getType("struct.C"))->size() == 1);
  ASSERT_TRUE(
      cxx_demangle(
          TH2.getVFTable(TH2.getType("struct.D"))->getFunction(0)->getName()) ==
      "A::f()");
  ASSERT_TRUE(TH2.getVFTable(TH2.getType("struct.D"))->size() == 1);
  ASSERT_TRUE(
      cxx_demangle(
          TH2.getVFTable(TH2.getType("struct.X"))->getFunction(0)->getName()) ==
      "X::g()");
  ASSERT_TRUE(

      TH2.getVFTable(TH2.getType("struct.X"))->size() == 1);
  ASSERT_TRUE(
      cxx_demangle(
          TH2.getVFTable(TH2.getType("struct.Y"))->getFunction(0)->getName()) ==
      "X::g()");
  ASSERT_TRUE(

      TH2.getVFTable(TH2.getType("struct.Y"))->size() == 1);
  ASSERT_TRUE(
      cxx_demangle(
          TH2.getVFTable(TH2.getType("struct.Z"))->getFunction(0)->getName()) ==
      "A::f()");
  ASSERT_TRUE(
      cxx_demangle(
          TH2.getVFTable(TH2.getType("struct.Z"))->getFunction(1)->getName()) ==
      "X::g()");
  ASSERT_TRUE(TH2.getVFTable(TH2.getType("struct.Z"))->size() == 2);

  ASSERT_TRUE(cxx_demangle(TH3.getVFTable(TH3.getType("struct.Base"))
                               ->getFunction(0)
                               ->getName()) == "Base::foo()");
  ASSERT_TRUE(cxx_demangle(TH3.getVFTable(TH3.getType("struct.Base"))
                               ->getFunction(1)
                               ->getName()) == "Base::bar()");
  ASSERT_TRUE(TH3.getVFTable(TH3.getType("struct.Base"))->size() == 2);
  ASSERT_TRUE(cxx_demangle(TH3.getVFTable(TH3.getType("struct.Child"))
                               ->getFunction(0)
                               ->getName()) == "Child::foo()");
  ASSERT_TRUE(cxx_demangle(TH3.getVFTable(TH3.getType("struct.Child"))
                               ->getFunction(1)
                               ->getName()) == "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH3.getVFTable(TH3.getType("struct.Child"))
                               ->getFunction(2)
                               ->getName()) == "Child::baz()");
  ASSERT_TRUE(TH3.getVFTable(TH3.getType("struct.Child"))->size() == 3);

  ASSERT_TRUE(cxx_demangle(TH4.getVFTable(TH4.getType("struct.Base"))
                               ->getFunction(0)
                               ->getName()) == "Base::foo()");
  ASSERT_TRUE(cxx_demangle(TH4.getVFTable(TH4.getType("struct.Base"))
                               ->getFunction(1)
                               ->getName()) == "Base::bar()");
  ASSERT_TRUE(TH4.getVFTable(TH4.getType("struct.Base"))->size() == 2);
  ASSERT_TRUE(cxx_demangle(TH4.getVFTable(TH4.getType("struct.Child"))
                               ->getFunction(0)
                               ->getName()) == "Child::foo()");
  ASSERT_TRUE(cxx_demangle(TH4.getVFTable(TH4.getType("struct.Child"))
                               ->getFunction(1)
                               ->getName()) == "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH4.getVFTable(TH4.getType("struct.Child"))
                               ->getFunction(2)
                               ->getName()) == "Child::baz()");
  ASSERT_TRUE(TH4.getVFTable(TH4.getType("struct.Child"))->size() == 3);

  ASSERT_TRUE(cxx_demangle(TH5.getVFTable(TH5.getType("struct.Base"))
                               ->getFunction(0)
                               ->getName()) == "__cxa_pure_virtual");
  ASSERT_TRUE(cxx_demangle(TH5.getVFTable(TH5.getType("struct.Base"))
                               ->getFunction(1)
                               ->getName()) == "Base::bar()");
  ASSERT_TRUE(TH5.getVFTable(TH5.getType("struct.Base"))->size() == 2);
  ASSERT_TRUE(cxx_demangle(TH5.getVFTable(TH5.getType("struct.Child"))
                               ->getFunction(0)
                               ->getName()) == "Child::foo()");
  ASSERT_TRUE(cxx_demangle(TH5.getVFTable(TH5.getType("struct.Child"))
                               ->getFunction(1)
                               ->getName()) == "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH5.getVFTable(TH5.getType("struct.Child"))
                               ->getFunction(2)
                               ->getName()) == "Child::baz()");
  ASSERT_TRUE(TH5.getVFTable(TH5.getType("struct.Child"))->size() == 3);
}

TEST_F(LTHTest, TransitivelyReachableTypes) {
  ProjectIRDB IRDB1(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_1_cpp.ll"});
  ProjectIRDB IRDB2(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_7_cpp.ll"});
  ProjectIRDB IRDB3(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_8_cpp.ll"});
  ProjectIRDB IRDB4(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_9_cpp.ll"});
  ProjectIRDB IRDB5(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_10_cpp.ll"});
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH1(IRDB1);
  LLVMTypeHierarchy TH2(IRDB2);
  LLVMTypeHierarchy TH3(IRDB3);
  LLVMTypeHierarchy TH4(IRDB4);
  LLVMTypeHierarchy TH5(IRDB5);

  auto reachable_types_base_1 = TH1.getSubTypes(TH1.getType("struct.Base"));
  auto reachable_types_child_1 = TH1.getSubTypes(TH1.getType("struct.Child"));

  auto reachable_types_A_2 = TH2.getSubTypes(TH2.getType("struct.A"));
  auto reachable_types_B_2 = TH2.getSubTypes(TH2.getType("struct.B"));
  auto reachable_types_C_2 = TH2.getSubTypes(TH2.getType("struct.C"));
  auto reachable_types_D_2 = TH2.getSubTypes(TH2.getType("struct.D"));
  auto reachable_types_X_2 = TH2.getSubTypes(TH2.getType("struct.X"));
  auto reachable_types_Y_2 = TH2.getSubTypes(TH2.getType("struct.Y"));
  auto reachable_types_Z_2 = TH2.getSubTypes(TH2.getType("struct.Z"));

  auto reachable_types_base_3 = TH3.getSubTypes(TH3.getType("struct.Base"));
  auto reachable_types_child_3 = TH3.getSubTypes(TH3.getType("struct.Child"));
  auto reachable_types_nonvirtualclass_3 =
      TH3.getSubTypes(TH3.getType("class.NonvirtualClass"));
  auto reachable_types_nonvirtualstruct_3 =
      TH3.getSubTypes(TH3.getType("struct.NonvirtualStruct"));

  auto reachable_types_base_4 = TH4.getSubTypes(TH4.getType("struct.Base"));
  auto reachable_types_child_4 = TH4.getSubTypes(TH4.getType("struct.Child"));

  auto reachable_types_base_5 = TH5.getSubTypes(TH5.getType("struct.Base"));
  auto reachable_types_child_5 = TH5.getSubTypes(TH5.getType("struct.Child"));

  // Will be way less dangerous to have an interface (like a map) between the
  // llvm given name of class & struct (i.e. struct.Base.base ...) and the name
  // inside phasar (i.e. just Base) and never work with the llvm name inside
  // phasar
  ASSERT_TRUE(reachable_types_base_1.count(TH1.getType("struct.Base")));
  ASSERT_TRUE(reachable_types_base_1.count(TH1.getType("struct.Child")));
  ASSERT_TRUE(reachable_types_base_1.size() == 2);
  ASSERT_FALSE(reachable_types_child_1.count(TH1.getType("struct.Base")));
  ASSERT_TRUE(reachable_types_child_1.count(TH1.getType("struct.Child")));
  ASSERT_TRUE(reachable_types_child_1.size() == 1);

  ASSERT_TRUE(reachable_types_A_2.count(TH2.getType("struct.A")));
  ASSERT_TRUE(reachable_types_A_2.count(TH2.getType("struct.B")));
  ASSERT_TRUE(reachable_types_A_2.count(TH2.getType("struct.C")));
  ASSERT_TRUE(reachable_types_A_2.count(TH2.getType("struct.D")));
  ASSERT_TRUE(reachable_types_A_2.count(TH2.getType("struct.Z")));
  ASSERT_TRUE(reachable_types_A_2.size() == 5);
  ASSERT_TRUE(reachable_types_B_2.count(TH2.getType("struct.B")));
  ASSERT_TRUE(reachable_types_B_2.count(TH2.getType("struct.D")));
  ASSERT_TRUE(reachable_types_B_2.size() == 2);
  ASSERT_TRUE(reachable_types_C_2.count(TH2.getType("struct.C")));
  ASSERT_TRUE(reachable_types_C_2.count(TH2.getType("struct.Z")));
  ASSERT_TRUE(reachable_types_C_2.size() == 2);
  ASSERT_TRUE(reachable_types_D_2.count(TH2.getType("struct.D")));
  ASSERT_TRUE(reachable_types_D_2.size() == 1);
  ASSERT_TRUE(reachable_types_X_2.count(TH2.getType("struct.X")));
  ASSERT_TRUE(reachable_types_X_2.count(TH2.getType("struct.Y")));
  ASSERT_TRUE(reachable_types_X_2.count(TH2.getType("struct.Z")));
  ASSERT_TRUE(reachable_types_X_2.size() == 3);
  ASSERT_TRUE(reachable_types_Y_2.count(TH2.getType("struct.Y")));
  ASSERT_TRUE(reachable_types_Y_2.count(TH2.getType("struct.Z")));
  ASSERT_TRUE(reachable_types_Y_2.size() == 2);
  ASSERT_TRUE(reachable_types_Z_2.count(TH2.getType("struct.Z")));
  ASSERT_TRUE(reachable_types_Z_2.size() == 1);

  ASSERT_TRUE(reachable_types_base_3.count(TH3.getType("struct.Base")));
  ASSERT_TRUE(reachable_types_base_3.count(TH3.getType("struct.Child")));
  ASSERT_TRUE(reachable_types_base_3.size() == 2);
  ASSERT_TRUE(reachable_types_child_3.count(TH3.getType("struct.Child")));
  ASSERT_TRUE(reachable_types_child_3.size() == 1);
  ASSERT_TRUE(reachable_types_nonvirtualclass_3.count(
      TH3.getType("class.NonvirtualClass")));
  ASSERT_TRUE(reachable_types_nonvirtualclass_3.size() == 1);
  ASSERT_TRUE(reachable_types_nonvirtualstruct_3.count(
      TH3.getType("struct.NonvirtualStruct")));
  ASSERT_TRUE(reachable_types_nonvirtualstruct_3.size() == 1);

  ASSERT_TRUE(reachable_types_base_4.count(TH4.getType("struct.Base")));
  ASSERT_FALSE(reachable_types_base_4.count(TH4.getType("struct.Base.base")));
  ASSERT_TRUE(reachable_types_base_4.count(TH4.getType("struct.Child")));
  ASSERT_TRUE(reachable_types_base_4.size() == 2);
  ASSERT_TRUE(reachable_types_child_4.count(TH4.getType("struct.Child")));
  ASSERT_TRUE(reachable_types_child_4.size() == 1);

  ASSERT_TRUE(reachable_types_base_5.count(TH5.getType("struct.Base")));
  ASSERT_TRUE(reachable_types_base_5.count(TH5.getType("struct.Child")));
  ASSERT_TRUE(reachable_types_base_5.size() == 2);
  ASSERT_TRUE(reachable_types_child_5.count(TH5.getType("struct.Child")));
  ASSERT_TRUE(reachable_types_child_5.size() == 1);
}

// TEST_F(LTHTest, HandleLoadAndPrintOfNonEmptyGraph) {
//   ProjectIRDB IRDB(
//       {pathToLLFiles + "type_hierarchies/type_hierarchy_1_cpp.ll"});
//   LLVMTypeHierarchy TH(IRDB);
//   TH.print(std::cout);
//   //   std::ostringstream oss;
//   //   // Write empty LTH graph as dot to string
//   //   TH.printGraphAsDot(oss);
//   //   oss.flush();
//   //   std::cout << oss.str() << std::endl;
//   //   std::string dot = oss.str();
//   //   // Reconstruct a LTH graph from the created dot file
//   //   std::istringstream iss(dot);
//   //   LLVMTypeHierarchy::bidigraph_t G =
//   //   LLVMTypeHierarchy::loadGraphFormDot(iss); boost::dynamic_properties
//   dp;
//   //   dp.property("node_id", get(&LLVMTypeHierarchy::VertexProperties::name,
//   //   G)); std::ostringstream oss2; boost::write_graphviz_dp(oss2, G, dp);
//   //   oss2.flush();
//   //   std::cout << oss2.str() << std::endl;
//   //   ASSERT_TRUE(boost::isomorphism(G, TH.TypeGraph));
// }

// // TEST_F(LTHTest, HandleLoadAndPrintOfEmptyGraph) {
// //   ProjectIRDB IRDB({pathToLLFiles +
// //   "taint_analysis/growing_example_cpp.ll"}); LLVMTypeHierarchy TH(IRDB);
// //   std::ostringstream oss;
// //   // Write empty LTH graph as dot to string
// //   TH.printGraphAsDot(oss);
// //   oss.flush();
// //   std::string dot = oss.str();
// //   // Reconstruct a LTH graph from the created dot file
// //   std::istringstream iss(dot);
// //   LLVMTypeHierarchy::bidigraph_t G =
// //   LLVMTypeHierarchy::loadGraphFormDot(iss); boost::dynamic_properties dp;
// //   dp.property("node_id", get(&LLVMTypeHierarchy::VertexProperties::name,
// G));
// //   std::ostringstream oss2;
// //   boost::write_graphviz_dp(oss2, G, dp);
// //   oss2.flush();
// //   ASSERT_EQ(oss.str(), oss2.str());
// // }

// // TEST_F(LTHTest, HandleMerge_1) {
// //   ProjectIRDB IRDB(
// //       {pathToLLFiles + "type_hierarchies/type_hierarchy_12_cpp.ll",
// //        pathToLLFiles + "type_hierarchies/type_hierarchy_12_b_cpp.ll"});
// //   LLVMTypeHierarchy TH1(*IRDB.getModule(
// //       pathToLLFiles + "type_hierarchies/type_hierarchy_12_cpp.ll"));
// //   LLVMTypeHierarchy TH2(*IRDB.getModule(
// //       pathToLLFiles + "type_hierarchies/type_hierarchy_12_b_cpp.ll"));
// //   TH1.mergeWith(TH2);
// //   TH1.print();
// //   EXPECT_TRUE(TH1.hasType(LTH.getType("class.Base")));
// //   EXPECT_TRUE(TH1.hasType(LTH.getType("struct.Child")));
// //   EXPECT_TRUE(TH1.hasType("struct.ChildsChild"));
// //   EXPECT_EQ(TH1.getNumTypes(), 3);
// //   EXPECT_TRUE(
// //       TH1.isSubType(LTH.getType("class.Base"),
// LTH.getType("struct.Child")));
// //   EXPECT_TRUE(TH1.isSubType(LTH.getType("class.Base"),
// //   "struct.ChildsChild"));
// //   EXPECT_TRUE(TH1.isSubType(LTH.getType("struct.Child"),
// //   "struct.ChildsChild")); EXPECT_TRUE(
// //       TH1.isSuperType(LTH.getType("struct.Child"),
// //       LTH.getType("class.Base")));
// //   EXPECT_TRUE(
// //       TH1.isSuperType("struct.ChildsChild", LTH.getType("struct.Child")));
// //   EXPECT_TRUE(TH1.isSuperType("struct.ChildsChild",
// //   LTH.getType("class.Base")));
// //   EXPECT_TRUE(TH1.hasVFTable(LTH.getType("class.Base")));
// //   EXPECT_TRUE(TH1.hasVFTable(LTH.getType("struct.Child")));
// //   EXPECT_TRUE(TH1.hasVFTable("struct.ChildsChild"));
// //   EXPECT_EQ(TH1.getVTableEntry(LTH.getType("class.Base"), 0),
// //   "_ZN4Base3fooEv");
// //   EXPECT_EQ(TH1.getVTableEntry(LTH.getType("struct.Child"), 0),
// //             "_ZN5Child3fooEv");
// //   EXPECT_EQ(TH1.getVTableEntry("struct.ChildsChild", 0),
// //             "_ZN11ChildsChild3fooEv");
// //   EXPECT_EQ(TH1.getNumVTableEntries(LTH.getType("class.Base")), 1);
// //   EXPECT_EQ(TH1.getNumVTableEntries(LTH.getType("struct.Child")), 1);
// //   EXPECT_EQ(TH1.getNumVTableEntries("struct.ChildsChild"), 1);
// //   EXPECT_EQ(TH1.getReachableSuperTypes(LTH.getType("class.Base")).size(),
// 3);
// // EXPECT_EQ(TH1.getReachableSuperTypes(LTH.getType("struct.Child")).size(),
// //   2); EXPECT_EQ(TH1.getReachableSuperTypes("struct.ChildsChild").size(),
// 1);
// //   auto BaseReachable =
// TH1.getReachableSuperTypes(LTH.getType("class.Base"));
// //   EXPECT_TRUE(BaseReachable.count(LTH.getType("class.Base")));
// //   EXPECT_TRUE(BaseReachable.count(LTH.getType("struct.Child")));
// //   EXPECT_TRUE(BaseReachable.count("struct.ChildsChild"));
// //   auto ChildReachable =
// //   TH1.getReachableSuperTypes(LTH.getType("struct.Child"));
// //   EXPECT_TRUE(ChildReachable.count(LTH.getType("struct.Child")));
// //   EXPECT_TRUE(ChildReachable.count("struct.ChildsChild"));
// //   auto ChildsChildReachable =
// //   TH1.getReachableSuperTypes("struct.ChildsChild");
// //   EXPECT_TRUE(ChildsChildReachable.count("struct.ChildsChild"));
// // }

TEST_F(LTHTest, HandleSTLString) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_13_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  EXPECT_EQ(TH.getAllTypes().size(), 4);
  EXPECT_TRUE(TH.hasType(TH.getType("class.std::__cxx11::basic_string")));
  EXPECT_TRUE(
      TH.hasType(TH.getType("struct.std::__cxx11::basic_string<char, "
                            "std::char_traits<char>, std::allocator<char> "
                            ">::_Alloc_hider")));
  EXPECT_TRUE(TH.hasType(TH.getType("union.anon")));
  EXPECT_TRUE(TH.hasType(TH.getType("class.std::allocator")));
  // (virtual) inheritance is not used in STL types
  EXPECT_FALSE(
      TH.isSubType(TH.getType("struct.std::__cxx11::basic_string<char, "
                              "std::char_traits<char>, std::allocator<char> "
                              ">::_Alloc_hider"),
                   TH.getType("class.std::__cxx11::basic_string")));
  EXPECT_FALSE(TH.isSubType(TH.getType("union.anon"),
                            TH.getType("class.std::__cxx11::basic_string")));
  EXPECT_FALSE(
      TH.isSuperType(TH.getType("class.std::__cxx11::basic_string"),
                     TH.getType("struct.std::__cxx11::basic_string<char, "
                                "std::char_traits<char>, std::allocator<char> "
                                ">::_Alloc_hider")));
  EXPECT_TRUE(TH.isSuperType(TH.getType("class.std::allocator"),
                             TH.getType("class.std::allocator")));
}

} // namespace psr

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto res = RUN_ALL_TESTS();
  llvm::llvm_shutdown();
  return res;
}
