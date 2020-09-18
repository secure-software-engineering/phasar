#include <iostream>

#include "boost/graph/graph_utility.hpp"
#include "boost/graph/graphviz.hpp"
#include "boost/graph/isomorphism.hpp"

#include "gtest/gtest.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

namespace psr {

// Check basic type hierarchy construction
TEST(LTHTest, BasicTHReconstruction_1) {
  ProjectIRDB IRDB({unittest::PathToLLTestFiles +
                    "type_hierarchies/type_hierarchy_1_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 2U);
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
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 1U);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 1U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1U);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST(LTHTest, BasicTHReconstruction_2) {
  ProjectIRDB IRDB({unittest::PathToLLTestFiles +
                    "type_hierarchies/type_hierarchy_2_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 2U);
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
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 1U);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 1U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1U);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST(LTHTest, BasicTHReconstruction_3) {
  ProjectIRDB IRDB({unittest::PathToLLTestFiles +
                    "type_hierarchies/type_hierarchy_3_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 2U);
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
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 2U);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 2U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1U);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST(LTHTest, BasicTHReconstruction_4) {
  ProjectIRDB IRDB({unittest::PathToLLTestFiles +
                    "type_hierarchies/type_hierarchy_4_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 2U);
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
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 2U);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 3U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1U);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST(LTHTest, BasicTHReconstruction_5) {
  ProjectIRDB IRDB({unittest::PathToLLTestFiles +
                    "type_hierarchies/type_hierarchy_5_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.OtherBase")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 3U);
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
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 2U);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.OtherBase"))->size(), 1U);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 5U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.OtherBase")).size(), 2U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1U);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto OtherBaseReachable = LTH.getSubTypes(LTH.getType("struct.OtherBase"));
  EXPECT_EQ(OtherBaseReachable.count(LTH.getType("struct.OtherBase")), true);
  EXPECT_EQ(OtherBaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST(LTHTest, BasicTHReconstruction_6) {
  ProjectIRDB IRDB({unittest::PathToLLTestFiles +
                    "type_hierarchies/type_hierarchy_12_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("class.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  EXPECT_EQ(LTH.getAllTypes().size(), 2U);
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
  EXPECT_EQ(LTH.getVFTable(LTH.getType("class.Base"))->size(), 1U);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 1U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("class.Base")).size(), 2U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1U);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("class.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("class.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

TEST(LTHTest, BasicTHReconstruction_7) {
  ProjectIRDB IRDB({unittest::PathToLLTestFiles +
                    "type_hierarchies/type_hierarchy_11_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Base")), true);
  EXPECT_EQ(LTH.hasType(LTH.getType("struct.Child")), true);
  // has three types because of padding (introduction of intermediate type)
  EXPECT_EQ(LTH.getAllTypes().size(), 3U);
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
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Base"))->size(), 1U);
  EXPECT_EQ(LTH.getVFTable(LTH.getType("struct.Child"))->size(), 1U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Base")).size(), 2U);
  EXPECT_EQ(LTH.getSubTypes(LTH.getType("struct.Child")).size(), 1U);
  auto BaseReachable = LTH.getSubTypes(LTH.getType("struct.Base"));
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Base")), true);
  EXPECT_EQ(BaseReachable.count(LTH.getType("struct.Child")), true);
  auto ChildReachable = LTH.getSubTypes(LTH.getType("struct.Child"));
  EXPECT_EQ(ChildReachable.count(LTH.getType("struct.Child")), true);
}

// check if the vtables are constructed correctly in more complex scenarios
TEST(LTHTest, VTableConstruction) {
  ProjectIRDB IRDB1({unittest::PathToLLTestFiles +
                     "type_hierarchies/type_hierarchy_1_cpp.ll"});
  ProjectIRDB IRDB2({unittest::PathToLLTestFiles +
                     "type_hierarchies/type_hierarchy_7_cpp.ll"});
  ProjectIRDB IRDB3({unittest::PathToLLTestFiles +
                     "type_hierarchies/type_hierarchy_8_cpp.ll"});
  ProjectIRDB IRDB4({unittest::PathToLLTestFiles +
                     "type_hierarchies/type_hierarchy_9_cpp.ll"});
  ProjectIRDB IRDB5({unittest::PathToLLTestFiles +
                     "type_hierarchies/type_hierarchy_10_cpp.ll"});

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

  ASSERT_TRUE(cxxDemangle(TH1.getVFTable(TH1.getType("struct.Base"))
                              ->getFunction(0)
                              ->getName()) == "Base::foo()");
  ASSERT_TRUE(TH1.getVFTable(TH1.getType("struct.Base"))->size() == 1U);
  ASSERT_TRUE(cxxDemangle(TH1.getVFTable(TH1.getType("struct.Child"))
                              ->getFunction(0)
                              ->getName()) == "Child::foo()");
  ASSERT_TRUE(TH1.getVFTable(TH1.getType("struct.Child"))->size() == 1U);

  ASSERT_TRUE(
      cxxDemangle(
          TH2.getVFTable(TH2.getType("struct.A"))->getFunction(0)->getName()) ==
      "A::f()");
  ASSERT_TRUE(TH2.getVFTable(TH2.getType("struct.A"))->size() == 1U);
  ASSERT_TRUE(
      cxxDemangle(
          TH2.getVFTable(TH2.getType("struct.B"))->getFunction(0)->getName()) ==
      "A::f()");
  ASSERT_TRUE(TH2.getVFTable(TH2.getType("struct.B"))->size() == 1U);
  ASSERT_TRUE(
      cxxDemangle(
          TH2.getVFTable(TH2.getType("struct.C"))->getFunction(0)->getName()) ==
      "A::f()");
  ASSERT_TRUE(

      TH2.getVFTable(TH2.getType("struct.C"))->size() == 1U);
  ASSERT_TRUE(
      cxxDemangle(
          TH2.getVFTable(TH2.getType("struct.D"))->getFunction(0)->getName()) ==
      "A::f()");
  ASSERT_TRUE(TH2.getVFTable(TH2.getType("struct.D"))->size() == 1U);
  ASSERT_TRUE(
      cxxDemangle(
          TH2.getVFTable(TH2.getType("struct.X"))->getFunction(0)->getName()) ==
      "X::g()");
  ASSERT_TRUE(

      TH2.getVFTable(TH2.getType("struct.X"))->size() == 1U);
  ASSERT_TRUE(
      cxxDemangle(
          TH2.getVFTable(TH2.getType("struct.Y"))->getFunction(0)->getName()) ==
      "X::g()");
  ASSERT_TRUE(

      TH2.getVFTable(TH2.getType("struct.Y"))->size() == 1U);
  ASSERT_TRUE(
      cxxDemangle(
          TH2.getVFTable(TH2.getType("struct.Z"))->getFunction(0)->getName()) ==
      "A::f()");
  ASSERT_TRUE(
      cxxDemangle(
          TH2.getVFTable(TH2.getType("struct.Z"))->getFunction(1)->getName()) ==
      "X::g()");
  ASSERT_TRUE(TH2.getVFTable(TH2.getType("struct.Z"))->size() == 2U);

  ASSERT_TRUE(cxxDemangle(TH3.getVFTable(TH3.getType("struct.Base"))
                              ->getFunction(0)
                              ->getName()) == "Base::foo()");
  ASSERT_TRUE(cxxDemangle(TH3.getVFTable(TH3.getType("struct.Base"))
                              ->getFunction(1)
                              ->getName()) == "Base::bar()");
  ASSERT_TRUE(TH3.getVFTable(TH3.getType("struct.Base"))->size() == 2U);
  ASSERT_TRUE(cxxDemangle(TH3.getVFTable(TH3.getType("struct.Child"))
                              ->getFunction(0)
                              ->getName()) == "Child::foo()");
  ASSERT_TRUE(cxxDemangle(TH3.getVFTable(TH3.getType("struct.Child"))
                              ->getFunction(1)
                              ->getName()) == "Base::bar()");
  ASSERT_TRUE(cxxDemangle(TH3.getVFTable(TH3.getType("struct.Child"))
                              ->getFunction(2)
                              ->getName()) == "Child::baz()");
  ASSERT_TRUE(TH3.getVFTable(TH3.getType("struct.Child"))->size() == 3U);

  ASSERT_TRUE(cxxDemangle(TH4.getVFTable(TH4.getType("struct.Base"))
                              ->getFunction(0)
                              ->getName()) == "Base::foo()");
  ASSERT_TRUE(cxxDemangle(TH4.getVFTable(TH4.getType("struct.Base"))
                              ->getFunction(1)
                              ->getName()) == "Base::bar()");
  ASSERT_TRUE(TH4.getVFTable(TH4.getType("struct.Base"))->size() == 2U);
  ASSERT_TRUE(cxxDemangle(TH4.getVFTable(TH4.getType("struct.Child"))
                              ->getFunction(0)
                              ->getName()) == "Child::foo()");
  ASSERT_TRUE(cxxDemangle(TH4.getVFTable(TH4.getType("struct.Child"))
                              ->getFunction(1)
                              ->getName()) == "Base::bar()");
  ASSERT_TRUE(cxxDemangle(TH4.getVFTable(TH4.getType("struct.Child"))
                              ->getFunction(2)
                              ->getName()) == "Child::baz()");
  ASSERT_TRUE(TH4.getVFTable(TH4.getType("struct.Child"))->size() == 3U);

  ASSERT_TRUE(cxxDemangle(TH5.getVFTable(TH5.getType("struct.Base"))
                              ->getFunction(0)
                              ->getName()) == "__cxa_pure_virtual");
  ASSERT_TRUE(cxxDemangle(TH5.getVFTable(TH5.getType("struct.Base"))
                              ->getFunction(1)
                              ->getName()) == "Base::bar()");
  ASSERT_TRUE(TH5.getVFTable(TH5.getType("struct.Base"))->size() == 2U);
  ASSERT_TRUE(cxxDemangle(TH5.getVFTable(TH5.getType("struct.Child"))
                              ->getFunction(0)
                              ->getName()) == "Child::foo()");
  ASSERT_TRUE(cxxDemangle(TH5.getVFTable(TH5.getType("struct.Child"))
                              ->getFunction(1)
                              ->getName()) == "Base::bar()");
  ASSERT_TRUE(cxxDemangle(TH5.getVFTable(TH5.getType("struct.Child"))
                              ->getFunction(2)
                              ->getName()) == "Child::baz()");
  ASSERT_TRUE(TH5.getVFTable(TH5.getType("struct.Child"))->size() == 3U);
}

TEST(LTHTest, TransitivelyReachableTypes) {
  ProjectIRDB IRDB1({unittest::PathToLLTestFiles +
                     "type_hierarchies/type_hierarchy_1_cpp.ll"});
  ProjectIRDB IRDB2({unittest::PathToLLTestFiles +
                     "type_hierarchies/type_hierarchy_7_cpp.ll"});
  ProjectIRDB IRDB3({unittest::PathToLLTestFiles +
                     "type_hierarchies/type_hierarchy_8_cpp.ll"});
  ProjectIRDB IRDB4({unittest::PathToLLTestFiles +
                     "type_hierarchies/type_hierarchy_9_cpp.ll"});
  ProjectIRDB IRDB5({unittest::PathToLLTestFiles +
                     "type_hierarchies/type_hierarchy_10_cpp.ll"});
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH1(IRDB1);
  LLVMTypeHierarchy TH2(IRDB2);
  LLVMTypeHierarchy TH3(IRDB3);
  LLVMTypeHierarchy TH4(IRDB4);
  LLVMTypeHierarchy TH5(IRDB5);

  auto ReachableTypesBase1 = TH1.getSubTypes(TH1.getType("struct.Base"));
  auto ReachableTypesChild1 = TH1.getSubTypes(TH1.getType("struct.Child"));

  auto ReachableTypesA2 = TH2.getSubTypes(TH2.getType("struct.A"));
  auto ReachableTypesB2 = TH2.getSubTypes(TH2.getType("struct.B"));
  auto ReachableTypesC2 = TH2.getSubTypes(TH2.getType("struct.C"));
  auto ReachableTypesD2 = TH2.getSubTypes(TH2.getType("struct.D"));
  auto ReachableTypesX2 = TH2.getSubTypes(TH2.getType("struct.X"));
  auto ReachableTypesY2 = TH2.getSubTypes(TH2.getType("struct.Y"));
  auto ReachableTypesZ2 = TH2.getSubTypes(TH2.getType("struct.Z"));

  auto ReachableTypesBase3 = TH3.getSubTypes(TH3.getType("struct.Base"));
  auto ReachableTypesChild3 = TH3.getSubTypes(TH3.getType("struct.Child"));
  auto ReachableTypesNonvirtualclass3 =
      TH3.getSubTypes(TH3.getType("class.NonvirtualClass"));
  auto ReachableTypesNonvirtualstruct3 =
      TH3.getSubTypes(TH3.getType("struct.NonvirtualStruct"));

  auto ReachableTypesBase4 = TH4.getSubTypes(TH4.getType("struct.Base"));
  auto ReachableTypesChild4 = TH4.getSubTypes(TH4.getType("struct.Child"));

  auto ReachableTypesBase5 = TH5.getSubTypes(TH5.getType("struct.Base"));
  auto ReachableTypesChild5 = TH5.getSubTypes(TH5.getType("struct.Child"));

  // Will be way less dangerous to have an interface (like a map) between the
  // llvm given name of class & struct (i.e. struct.Base.base ...) and the name
  // inside phasar (i.e. just Base) and never work with the llvm name inside
  // phasar
  ASSERT_TRUE(ReachableTypesBase1.count(TH1.getType("struct.Base")));
  ASSERT_TRUE(ReachableTypesBase1.count(TH1.getType("struct.Child")));
  ASSERT_TRUE(ReachableTypesBase1.size() == 2U);
  ASSERT_FALSE(ReachableTypesChild1.count(TH1.getType("struct.Base")));
  ASSERT_TRUE(ReachableTypesChild1.count(TH1.getType("struct.Child")));
  ASSERT_TRUE(ReachableTypesChild1.size() == 1U);

  ASSERT_TRUE(ReachableTypesA2.count(TH2.getType("struct.A")));
  ASSERT_TRUE(ReachableTypesA2.count(TH2.getType("struct.B")));
  ASSERT_TRUE(ReachableTypesA2.count(TH2.getType("struct.C")));
  ASSERT_TRUE(ReachableTypesA2.count(TH2.getType("struct.D")));
  ASSERT_TRUE(ReachableTypesA2.count(TH2.getType("struct.Z")));
  ASSERT_TRUE(ReachableTypesA2.size() == 5U);
  ASSERT_TRUE(ReachableTypesB2.count(TH2.getType("struct.B")));
  ASSERT_TRUE(ReachableTypesB2.count(TH2.getType("struct.D")));
  ASSERT_TRUE(ReachableTypesB2.size() == 2U);
  ASSERT_TRUE(ReachableTypesC2.count(TH2.getType("struct.C")));
  ASSERT_TRUE(ReachableTypesC2.count(TH2.getType("struct.Z")));
  ASSERT_TRUE(ReachableTypesC2.size() == 2U);
  ASSERT_TRUE(ReachableTypesD2.count(TH2.getType("struct.D")));
  ASSERT_TRUE(ReachableTypesD2.size() == 1U);
  ASSERT_TRUE(ReachableTypesX2.count(TH2.getType("struct.X")));
  ASSERT_TRUE(ReachableTypesX2.count(TH2.getType("struct.Y")));
  ASSERT_TRUE(ReachableTypesX2.count(TH2.getType("struct.Z")));
  ASSERT_TRUE(ReachableTypesX2.size() == 3U);
  ASSERT_TRUE(ReachableTypesY2.count(TH2.getType("struct.Y")));
  ASSERT_TRUE(ReachableTypesY2.count(TH2.getType("struct.Z")));
  ASSERT_TRUE(ReachableTypesY2.size() == 2U);
  ASSERT_TRUE(ReachableTypesZ2.count(TH2.getType("struct.Z")));
  ASSERT_TRUE(ReachableTypesZ2.size() == 1U);

  ASSERT_TRUE(ReachableTypesBase3.count(TH3.getType("struct.Base")));
  ASSERT_TRUE(ReachableTypesBase3.count(TH3.getType("struct.Child")));
  ASSERT_TRUE(ReachableTypesBase3.size() == 2U);
  ASSERT_TRUE(ReachableTypesChild3.count(TH3.getType("struct.Child")));
  ASSERT_TRUE(ReachableTypesChild3.size() == 1U);
  ASSERT_TRUE(ReachableTypesNonvirtualclass3.count(
      TH3.getType("class.NonvirtualClass")));
  ASSERT_TRUE(ReachableTypesNonvirtualclass3.size() == 1U);
  ASSERT_TRUE(ReachableTypesNonvirtualstruct3.count(
      TH3.getType("struct.NonvirtualStruct")));
  ASSERT_TRUE(ReachableTypesNonvirtualstruct3.size() == 1U);

  ASSERT_TRUE(ReachableTypesBase4.count(TH4.getType("struct.Base")));
  ASSERT_FALSE(ReachableTypesBase4.count(TH4.getType("struct.Base.base")));
  ASSERT_TRUE(ReachableTypesBase4.count(TH4.getType("struct.Child")));
  ASSERT_TRUE(ReachableTypesBase4.size() == 2U);
  ASSERT_TRUE(ReachableTypesChild4.count(TH4.getType("struct.Child")));
  ASSERT_TRUE(ReachableTypesChild4.size() == 1U);

  ASSERT_TRUE(ReachableTypesBase5.count(TH5.getType("struct.Base")));
  ASSERT_TRUE(ReachableTypesBase5.count(TH5.getType("struct.Child")));
  ASSERT_TRUE(ReachableTypesBase5.size() == 2U);
  ASSERT_TRUE(ReachableTypesChild5.count(TH5.getType("struct.Child")));
  ASSERT_TRUE(ReachableTypesChild5.size() == 1U);
}

// TEST(LTHTest, HandleLoadAndPrintOfNonEmptyGraph) {
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

// // TEST(LTHTest, HandleLoadAndPrintOfEmptyGraph) {
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

// // TEST(LTHTest, HandleMerge_1) {
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
// 3U);
// // EXPECT_EQ(TH1.getReachableSuperTypes(LTH.getType("struct.Child")).size(),
// //   2U); EXPECT_EQ(TH1.getReachableSuperTypes("struct.ChildsChild").size(),
// 1U);
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

TEST(LTHTest, HandleSTLString) {
  ProjectIRDB IRDB({unittest::PathToLLTestFiles +
                    "type_hierarchies/type_hierarchy_13_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  EXPECT_EQ(TH.getAllTypes().size(), 4U);
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

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  auto Res = RUN_ALL_TESTS();
  llvm::llvm_shutdown();
  return Res;
}
