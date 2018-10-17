#include <iostream>

#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/isomorphism.hpp>

#include <gtest/gtest.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Macros.h>

using namespace std;
using namespace psr;

namespace psr {
class LTHTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/";
};

// Check basic type hierarchy construction
TEST_F(LTHTest, BasicTHReconstruction_1) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_1_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.containsType("struct.Base"), true);
  EXPECT_EQ(LTH.containsType("struct.Child"), true);
  EXPECT_EQ(LTH.getNumTypes(), 2);
  EXPECT_EQ(LTH.hasSubType("struct.Base", "struct.Child"), true);
  EXPECT_EQ(LTH.hasSuperType("struct.Child", "struct.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Child"), true);
  EXPECT_EQ(LTH.getVTableEntry("struct.Base", 0), "_ZN4Base3fooEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 0), "_ZN5Child3fooEv");
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Base"), 1);
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Child"), 1);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Base").size(), 2);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Child").size(), 1);
  auto BaseReachable = LTH.getTransitivelyReachableTypes("struct.Base");
  EXPECT_EQ(BaseReachable.count("struct.Base"), true);
  EXPECT_EQ(BaseReachable.count("struct.Child"), true);
  auto ChildReachable = LTH.getTransitivelyReachableTypes("struct.Child");
  EXPECT_EQ(ChildReachable.count("struct.Child"), true);
}

TEST_F(LTHTest, BasicTHReconstruction_2) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_2_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.containsType("struct.Base"), true);
  EXPECT_EQ(LTH.containsType("struct.Child"), true);
  EXPECT_EQ(LTH.getNumTypes(), 2);
  EXPECT_EQ(LTH.hasSubType("struct.Base", "struct.Child"), true);
  EXPECT_EQ(LTH.hasSuperType("struct.Child", "struct.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Child"), true);
  EXPECT_EQ(LTH.getVTableEntry("struct.Base", 0), "_ZN4Base3fooEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 0), "_ZN5Child3fooEv");
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Base"), 1);
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Child"), 1);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Base").size(), 2);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Child").size(), 1);
  auto BaseReachable = LTH.getTransitivelyReachableTypes("struct.Base");
  EXPECT_EQ(BaseReachable.count("struct.Base"), true);
  EXPECT_EQ(BaseReachable.count("struct.Child"), true);
  auto ChildReachable = LTH.getTransitivelyReachableTypes("struct.Child");
  EXPECT_EQ(ChildReachable.count("struct.Child"), true);
}

TEST_F(LTHTest, BasicTHReconstruction_3) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_3_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.containsType("struct.Base"), true);
  EXPECT_EQ(LTH.containsType("struct.Child"), true);
  EXPECT_EQ(LTH.getNumTypes(), 2);
  EXPECT_EQ(LTH.hasSubType("struct.Base", "struct.Child"), true);
  EXPECT_EQ(LTH.hasSuperType("struct.Child", "struct.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Child"), true);
  EXPECT_EQ(LTH.getVTableEntry("struct.Base", 0), "_ZN4Base3fooEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Base", 1), "_ZN4Base3barEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 0), "_ZN5Child3fooEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 1), "_ZN4Base3barEv");
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Base"), 2);
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Child"), 2);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Base").size(), 2);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Child").size(), 1);
  auto BaseReachable = LTH.getTransitivelyReachableTypes("struct.Base");
  EXPECT_EQ(BaseReachable.count("struct.Base"), true);
  EXPECT_EQ(BaseReachable.count("struct.Child"), true);
  auto ChildReachable = LTH.getTransitivelyReachableTypes("struct.Child");
  EXPECT_EQ(ChildReachable.count("struct.Child"), true);
}

TEST_F(LTHTest, BasicTHReconstruction_4) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_4_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.containsType("struct.Base"), true);
  EXPECT_EQ(LTH.containsType("struct.Child"), true);
  EXPECT_EQ(LTH.getNumTypes(), 2);
  EXPECT_EQ(LTH.hasSubType("struct.Base", "struct.Child"), true);
  EXPECT_EQ(LTH.hasSuperType("struct.Child", "struct.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Child"), true);
  EXPECT_EQ(LTH.getVTableEntry("struct.Base", 0), "_ZN4Base3fooEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Base", 1), "_ZN4Base3barEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 0), "_ZN5Child3fooEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 1), "_ZN4Base3barEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 2), "_ZN5Child3tarEv");
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Base"), 2);
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Child"), 3);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Base").size(), 2);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Child").size(), 1);
  auto BaseReachable = LTH.getTransitivelyReachableTypes("struct.Base");
  EXPECT_EQ(BaseReachable.count("struct.Base"), true);
  EXPECT_EQ(BaseReachable.count("struct.Child"), true);
  auto ChildReachable = LTH.getTransitivelyReachableTypes("struct.Child");
  EXPECT_EQ(ChildReachable.count("struct.Child"), true);
}

TEST_F(LTHTest, BasicTHReconstruction_5) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_5_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.containsType("struct.Base"), true);
  EXPECT_EQ(LTH.containsType("struct.Child"), true);
  EXPECT_EQ(LTH.containsType("struct.OtherBase"), true);
  EXPECT_EQ(LTH.getNumTypes(), 3);
  EXPECT_EQ(LTH.hasSubType("struct.Base", "struct.Child"), true);
  EXPECT_EQ(LTH.hasSubType("struct.OtherBase", "struct.Child"), true);
  EXPECT_EQ(LTH.hasSuperType("struct.Child", "struct.Base"), true);
  EXPECT_EQ(LTH.hasSuperType("struct.Child", "struct.OtherBase"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.OtherBase"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Child"), true);
  EXPECT_EQ(LTH.getVTableEntry("struct.Base", 0), "_ZN4Base3fooEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Base", 1), "_ZN4Base3barEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.OtherBase", 0), "_ZN9OtherBase3bazEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 0), "_ZN5Child3fooEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 1), "_ZN4Base3barEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 2), "_ZN5Child3bazEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 3), "_ZN5Child3tarEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 4), "_ZThn8_N5Child3bazEv");
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Base"), 2);
  EXPECT_EQ(LTH.getNumVTableEntries("struct.OtherBase"), 1);
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Child"), 5);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Base").size(), 2);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.OtherBase").size(), 2);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Child").size(), 1);
  auto BaseReachable = LTH.getTransitivelyReachableTypes("struct.Base");
  EXPECT_EQ(BaseReachable.count("struct.Base"), true);
  EXPECT_EQ(BaseReachable.count("struct.Child"), true);
  auto OtherBaseReachable =
      LTH.getTransitivelyReachableTypes("struct.OtherBase");
  EXPECT_EQ(OtherBaseReachable.count("struct.OtherBase"), true);
  EXPECT_EQ(OtherBaseReachable.count("struct.Child"), true);
  auto ChildReachable = LTH.getTransitivelyReachableTypes("struct.Child");
  EXPECT_EQ(ChildReachable.count("struct.Child"), true);
}

TEST_F(LTHTest, BasicTHReconstruction_6) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_12_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.containsType("class.Base"), true);
  EXPECT_EQ(LTH.containsType("struct.Child"), true);
  EXPECT_EQ(LTH.getNumTypes(), 2);
  EXPECT_EQ(LTH.hasSubType("class.Base", "struct.Child"), true);
  EXPECT_EQ(LTH.hasSuperType("struct.Child", "class.Base"), true);
  EXPECT_EQ(LTH.containsVTable("class.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Child"), true);
  EXPECT_EQ(LTH.getVTableEntry("class.Base", 0), "_ZN4Base3fooEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 0), "_ZN5Child3fooEv");
  EXPECT_EQ(LTH.getNumVTableEntries("class.Base"), 1);
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Child"), 1);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("class.Base").size(), 2);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Child").size(), 1);
  auto BaseReachable = LTH.getTransitivelyReachableTypes("class.Base");
  EXPECT_EQ(BaseReachable.count("class.Base"), true);
  EXPECT_EQ(BaseReachable.count("struct.Child"), true);
  auto ChildReachable = LTH.getTransitivelyReachableTypes("struct.Child");
  EXPECT_EQ(ChildReachable.count("struct.Child"), true);
}

TEST_F(LTHTest, BasicTHReconstruction_7) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_11_cpp.ll"});
  LLVMTypeHierarchy LTH(IRDB);
  EXPECT_EQ(LTH.containsType("struct.Base"), true);
  EXPECT_EQ(LTH.containsType("struct.Child"), true);
  EXPECT_EQ(LTH.getNumTypes(), 2);
  EXPECT_EQ(LTH.hasSubType("struct.Base", "struct.Child"), true);
  EXPECT_EQ(LTH.hasSuperType("struct.Child", "struct.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Base"), true);
  EXPECT_EQ(LTH.containsVTable("struct.Child"), true);
  EXPECT_EQ(LTH.getVTableEntry("struct.Base", 0), "_ZN4Base3fooEv");
  EXPECT_EQ(LTH.getVTableEntry("struct.Child", 0), "_ZN5Child3fooEv");
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Base"), 1);
  EXPECT_EQ(LTH.getNumVTableEntries("struct.Child"), 1);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Base").size(), 2);
  EXPECT_EQ(LTH.getTransitivelyReachableTypes("struct.Child").size(), 1);
  auto BaseReachable = LTH.getTransitivelyReachableTypes("struct.Base");
  EXPECT_EQ(BaseReachable.count("struct.Base"), true);
  EXPECT_EQ(BaseReachable.count("struct.Child"), true);
  auto ChildReachable = LTH.getTransitivelyReachableTypes("struct.Child");
  EXPECT_EQ(ChildReachable.count("struct.Child"), true);
}

// more complex scenarios and situations involving several LLVM IR files
TEST_F(LTHTest, GraphConstruction) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_1_cpp.ll",
       pathToLLFiles + "type_hierarchies/type_hierarchy_7_cpp.ll",
       pathToLLFiles + "type_hierarchies/type_hierarchy_8_cpp.ll",
       pathToLLFiles + "type_hierarchies/type_hierarchy_9_cpp.ll",
       pathToLLFiles + "type_hierarchies/type_hierarchy_10_cpp.ll"});
  llvm::Module *M = IRDB.getModule(pathToLLFiles +
                                   "type_hierarchies/type_hierarchy_1_cpp.ll");
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH1(*M);

  LLVMTypeHierarchy::bidigraph_t graph1;
  auto base_vertex_1 = boost::add_vertex(graph1);
  auto child_vertex_1 = boost::add_vertex(graph1);
  boost::add_edge(child_vertex_1, base_vertex_1, graph1);

  ASSERT_TRUE(boost::isomorphism(graph1, TH1.g));

  M = IRDB.getModule(pathToLLFiles +
                     "type_hierarchies/type_hierarchy_7_cpp.ll");
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH2(*M);

  LLVMTypeHierarchy::bidigraph_t graph2;
  auto A_vertex_2 = boost::add_vertex(graph2);
  auto B_vertex_2 = boost::add_vertex(graph2);
  auto C_vertex_2 = boost::add_vertex(graph2);
  auto D_vertex_2 = boost::add_vertex(graph2);
  auto X_vertex_2 = boost::add_vertex(graph2);
  auto Y_vertex_2 = boost::add_vertex(graph2);
  auto Z_vertex_2 = boost::add_vertex(graph2);

  boost::add_edge(B_vertex_2, A_vertex_2, graph2);
  boost::add_edge(C_vertex_2, A_vertex_2, graph2);
  boost::add_edge(D_vertex_2, B_vertex_2, graph2);
  boost::add_edge(Y_vertex_2, X_vertex_2, graph2);
  boost::add_edge(Z_vertex_2, C_vertex_2, graph2);
  boost::add_edge(Z_vertex_2, Y_vertex_2, graph2);

  ASSERT_TRUE(boost::isomorphism(graph2, TH2.g));

  M = IRDB.getModule(pathToLLFiles +
                     "type_hierarchies/type_hierarchy_8_cpp.ll");
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH3(*M);

  LLVMTypeHierarchy::bidigraph_t graph3;
  auto base_vertex_3 = boost::add_vertex(graph3);
  auto child_vertex_3 = boost::add_vertex(graph3);
  auto NonvirtualClass_vertex_3 = boost::add_vertex(graph3);
  auto NonvirtualStruct_vertex_3 = boost::add_vertex(graph3);

  boost::add_edge(child_vertex_3, base_vertex_3, graph3);

  ASSERT_TRUE(boost::isomorphism(graph3, TH3.g));

  M = IRDB.getModule(pathToLLFiles +
                     "type_hierarchies/type_hierarchy_9_cpp.ll");
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH4(*M);

  LLVMTypeHierarchy::bidigraph_t graph4;
  auto base_vertex_4 = boost::add_vertex(graph4);
  auto child_vertex_4 = boost::add_vertex(graph4);

  boost::add_edge(child_vertex_4, base_vertex_4, graph4);

  ASSERT_TRUE(boost::isomorphism(graph4, TH4.g));

  M = IRDB.getModule(pathToLLFiles +
                     "type_hierarchies/type_hierarchy_10_cpp.ll");
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH5(*M);

  LLVMTypeHierarchy::bidigraph_t graph5;
  auto base_vertex_5 = boost::add_vertex(graph5);
  auto child_vertex_5 = boost::add_vertex(graph5);

  boost::add_edge(child_vertex_5, base_vertex_5, graph5);

  ASSERT_TRUE(boost::isomorphism(graph5, TH5.g));
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

  ASSERT_TRUE(TH1.containsVTable("struct.Base"));
  ASSERT_TRUE(TH1.containsVTable("struct.Child"));
  ASSERT_FALSE(TH1.containsVTable("struct.ANYTHING"));

  ASSERT_TRUE(TH2.containsVTable("struct.A"));
  ASSERT_TRUE(TH2.containsVTable("struct.B"));
  ASSERT_TRUE(TH2.containsVTable("struct.C"));
  ASSERT_TRUE(TH2.containsVTable("struct.D"));
  ASSERT_TRUE(TH2.containsVTable("struct.X"));
  ASSERT_TRUE(TH2.containsVTable("struct.Y"));
  ASSERT_TRUE(TH2.containsVTable("struct.Z"));

  ASSERT_TRUE(TH3.containsVTable("struct.Base"));
  ASSERT_TRUE(TH3.containsVTable("struct.Child"));
  ASSERT_FALSE(TH3.containsVTable("class.NonvirtualClass"));
  ASSERT_FALSE(TH3.containsVTable("struct.NonvirtualStruct"));

  ASSERT_TRUE(TH4.containsVTable("struct.Base"));
  ASSERT_TRUE(TH4.containsVTable("struct.Child"));

  ASSERT_TRUE(TH5.containsVTable("struct.Base"));
  ASSERT_TRUE(TH5.containsVTable("struct.Child"));

  ASSERT_TRUE(cxx_demangle(TH1.getVTableEntry("struct.Base", 0)) ==
              "Base::foo()");
  ASSERT_TRUE(cxx_demangle(TH1.getVTableEntry("struct.Base", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH1.getVTableEntry("struct.Child", 0)) ==
              "Child::foo()");
  ASSERT_TRUE(cxx_demangle(TH1.getVTableEntry("struct.Child", 1)) == "");

  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.A", 0)) == "A::f()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.A", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.B", 0)) == "A::f()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.B", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.C", 0)) == "A::f()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.C", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.D", 0)) == "A::f()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.D", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.X", 0)) == "X::g()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.X", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.Y", 0)) == "X::g()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.Y", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.Z", 0)) == "A::f()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.Z", 1)) == "X::g()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("struct.Z", 2)) == "");

  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("struct.Base", 0)) ==
              "Base::foo()");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("struct.Base", 1)) ==
              "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("struct.Base", 2)) == "");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("struct.Child", 0)) ==
              "Child::foo()");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("struct.Child", 1)) ==
              "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("struct.Child", 2)) ==
              "Child::baz()");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("struct.Child", 3)) == "");

  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("struct.Base", 0)) ==
              "Base::foo()");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("struct.Base", 1)) ==
              "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("struct.Base", 2)) == "");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("struct.Child", 0)) ==
              "Child::foo()");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("struct.Child", 1)) ==
              "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("struct.Child", 2)) ==
              "Child::baz()");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("struct.Child", 3)) == "");

  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("struct.Base", 0)) ==
              "__cxa_pure_virtual");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("struct.Base", 1)) ==
              "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("struct.Base", 2)) == "");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("struct.Child", 0)) ==
              "Child::foo()");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("struct.Child", 1)) ==
              "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("struct.Child", 2)) ==
              "Child::baz()");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("struct.Child", 3)) == "");
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

  auto reachable_types_base_1 =
      TH1.getTransitivelyReachableTypes("struct.Base");
  auto reachable_types_child_1 =
      TH1.getTransitivelyReachableTypes("struct.Child");

  auto reachable_types_A_2 = TH2.getTransitivelyReachableTypes("struct.A");
  auto reachable_types_B_2 = TH2.getTransitivelyReachableTypes("struct.B");
  auto reachable_types_C_2 = TH2.getTransitivelyReachableTypes("struct.C");
  auto reachable_types_D_2 = TH2.getTransitivelyReachableTypes("struct.D");
  auto reachable_types_X_2 = TH2.getTransitivelyReachableTypes("struct.X");
  auto reachable_types_Y_2 = TH2.getTransitivelyReachableTypes("struct.Y");
  auto reachable_types_Z_2 = TH2.getTransitivelyReachableTypes("struct.Z");

  auto reachable_types_base_3 =
      TH3.getTransitivelyReachableTypes("struct.Base");
  auto reachable_types_child_3 =
      TH3.getTransitivelyReachableTypes("struct.Child");
  auto reachable_types_nonvirtualclass_3 =
      TH3.getTransitivelyReachableTypes("class.NonvirtualClass");
  auto reachable_types_nonvirtualstruct_3 =
      TH3.getTransitivelyReachableTypes("struct.NonvirtualStruct");

  auto reachable_types_base_4 =
      TH4.getTransitivelyReachableTypes("struct.Base");
  auto reachable_types_child_4 =
      TH4.getTransitivelyReachableTypes("struct.Child");

  auto reachable_types_base_5 =
      TH5.getTransitivelyReachableTypes("struct.Base");
  auto reachable_types_child_5 =
      TH5.getTransitivelyReachableTypes("struct.Child");

  // Will be way less dangerous to have an interface (like a map) between the
  // llvm given name of class & struct (i.e. struct.Base.base ...) and the name
  // inside phasar (i.e. just Base) and never work with the llvm name inside
  // phasar
  ASSERT_TRUE(reachable_types_base_1.count("struct.Base"));
  ASSERT_TRUE(reachable_types_base_1.count("struct.Child"));
  ASSERT_TRUE(reachable_types_base_1.size() == 2);
  ASSERT_FALSE(reachable_types_child_1.count("struct.Base"));
  ASSERT_TRUE(reachable_types_child_1.count("struct.Child"));
  ASSERT_TRUE(reachable_types_child_1.size() == 1);

  ASSERT_TRUE(reachable_types_A_2.count("struct.A"));
  ASSERT_TRUE(reachable_types_A_2.count("struct.B"));
  ASSERT_TRUE(reachable_types_A_2.count("struct.C"));
  ASSERT_TRUE(reachable_types_A_2.count("struct.D"));
  ASSERT_TRUE(reachable_types_A_2.count("struct.Z"));
  ASSERT_TRUE(reachable_types_A_2.size() == 5);
  ASSERT_TRUE(reachable_types_B_2.count("struct.B"));
  ASSERT_TRUE(reachable_types_B_2.count("struct.D"));
  ASSERT_TRUE(reachable_types_B_2.size() == 2);
  ASSERT_TRUE(reachable_types_C_2.count("struct.C"));
  ASSERT_TRUE(reachable_types_C_2.count("struct.Z"));
  ASSERT_TRUE(reachable_types_C_2.size() == 2);
  ASSERT_TRUE(reachable_types_D_2.count("struct.D"));
  ASSERT_TRUE(reachable_types_D_2.size() == 1);
  ASSERT_TRUE(reachable_types_X_2.count("struct.X"));
  ASSERT_TRUE(reachable_types_X_2.count("struct.Y"));
  ASSERT_TRUE(reachable_types_X_2.count("struct.Z"));
  ASSERT_TRUE(reachable_types_X_2.size() == 3);
  ASSERT_TRUE(reachable_types_Y_2.count("struct.Y"));
  ASSERT_TRUE(reachable_types_Y_2.count("struct.Z"));
  ASSERT_TRUE(reachable_types_Y_2.size() == 2);
  ASSERT_TRUE(reachable_types_Z_2.count("struct.Z"));
  ASSERT_TRUE(reachable_types_Z_2.size() == 1);

  ASSERT_TRUE(reachable_types_base_3.count("struct.Base"));
  ASSERT_TRUE(reachable_types_base_3.count("struct.Child"));
  ASSERT_TRUE(reachable_types_base_3.size() == 2);
  ASSERT_TRUE(reachable_types_child_3.count("struct.Child"));
  ASSERT_TRUE(reachable_types_child_3.size() == 1);
  ASSERT_TRUE(reachable_types_nonvirtualclass_3.count("class.NonvirtualClass"));
  ASSERT_TRUE(reachable_types_nonvirtualclass_3.size() == 1);
  ASSERT_TRUE(
      reachable_types_nonvirtualstruct_3.count("struct.NonvirtualStruct"));
  ASSERT_TRUE(reachable_types_nonvirtualstruct_3.size() == 1);

  ASSERT_TRUE(reachable_types_base_4.count("struct.Base"));
  ASSERT_FALSE(reachable_types_base_4.count("struct.Base.base"));
  ASSERT_TRUE(reachable_types_base_4.count("struct.Child"));
  ASSERT_TRUE(reachable_types_base_4.size() == 2);
  ASSERT_TRUE(reachable_types_child_4.count("struct.Child"));
  ASSERT_TRUE(reachable_types_child_4.size() == 1);

  ASSERT_TRUE(reachable_types_base_5.count("struct.Base"));
  ASSERT_TRUE(reachable_types_base_5.count("struct.Child"));
  ASSERT_TRUE(reachable_types_base_5.size() == 2);
  ASSERT_TRUE(reachable_types_child_5.count("struct.Child"));
  ASSERT_TRUE(reachable_types_child_5.size() == 1);
}

TEST_F(LTHTest, HandleLoadAndPrintOfNonEmptyGraph) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_1_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  TH.print();
  std::ostringstream oss;
  // Write empty LTH graph as dot to string
  TH.printGraphAsDot(oss);
  oss.flush();
  std::cout << oss.str() << std::endl;
  std::string dot = oss.str();
  // Reconstruct a LTH graph from the created dot file
  std::istringstream iss(dot);
  LLVMTypeHierarchy::bidigraph_t G = LLVMTypeHierarchy::loadGraphFormDot(iss);
  boost::dynamic_properties dp;
  dp.property("node_id", get(&LLVMTypeHierarchy::VertexProperties::name, G));
  std::ostringstream oss2;
  boost::write_graphviz_dp(oss2, G, dp);
  oss2.flush();
  std::cout << oss2.str() << std::endl;
  ASSERT_TRUE(boost::isomorphism(G, TH.g));
}

TEST_F(LTHTest, HandleLoadAndPrintOfEmptyGraph) {
  ProjectIRDB IRDB({pathToLLFiles + "taint_analysis/growing_example_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  std::ostringstream oss;
  // Write empty LTH graph as dot to string
  TH.printGraphAsDot(oss);
  oss.flush();
  std::string dot = oss.str();
  // Reconstruct a LTH graph from the created dot file
  std::istringstream iss(dot);
  LLVMTypeHierarchy::bidigraph_t G = LLVMTypeHierarchy::loadGraphFormDot(iss);
  boost::dynamic_properties dp;
  dp.property("node_id", get(&LLVMTypeHierarchy::VertexProperties::name, G));
  std::ostringstream oss2;
  boost::write_graphviz_dp(oss2, G, dp);
  oss2.flush();
  ASSERT_EQ(oss.str(), oss2.str());
}

TEST_F(LTHTest, HandleMerge_1) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_12_cpp.ll",
       pathToLLFiles + "type_hierarchies/type_hierarchy_12_b_cpp.ll"});
  LLVMTypeHierarchy TH1(*IRDB.getModule(
      pathToLLFiles + "type_hierarchies/type_hierarchy_12_cpp.ll"));
  LLVMTypeHierarchy TH2(*IRDB.getModule(
      pathToLLFiles + "type_hierarchies/type_hierarchy_12_b_cpp.ll"));
  TH1.mergeWith(TH2);
  TH1.print();
  EXPECT_TRUE(TH1.containsType("class.Base"));
  EXPECT_TRUE(TH1.containsType("struct.Child"));
  EXPECT_TRUE(TH1.containsType("struct.ChildsChild"));
  EXPECT_EQ(TH1.getNumTypes(), 3);
  EXPECT_TRUE(TH1.hasSubType("class.Base", "struct.Child"));
  EXPECT_TRUE(TH1.hasSubType("class.Base", "struct.ChildsChild"));
  EXPECT_TRUE(TH1.hasSubType("struct.Child", "struct.ChildsChild"));
  EXPECT_TRUE(TH1.hasSuperType("struct.Child", "class.Base"));
  EXPECT_TRUE(TH1.hasSuperType("struct.ChildsChild", "struct.Child"));
  EXPECT_TRUE(TH1.hasSuperType("struct.ChildsChild", "class.Base"));
  EXPECT_TRUE(TH1.containsVTable("class.Base"));
  EXPECT_TRUE(TH1.containsVTable("struct.Child"));
  EXPECT_TRUE(TH1.containsVTable("struct.ChildsChild"));
  EXPECT_EQ(TH1.getVTableEntry("class.Base", 0), "_ZN4Base3fooEv");
  EXPECT_EQ(TH1.getVTableEntry("struct.Child", 0), "_ZN5Child3fooEv");
  EXPECT_EQ(TH1.getVTableEntry("struct.ChildsChild", 0),
            "_ZN11ChildsChild3fooEv");
  EXPECT_EQ(TH1.getNumVTableEntries("class.Base"), 1);
  EXPECT_EQ(TH1.getNumVTableEntries("struct.Child"), 1);
  EXPECT_EQ(TH1.getNumVTableEntries("struct.ChildsChild"), 1);
  EXPECT_EQ(TH1.getTransitivelyReachableTypes("class.Base").size(), 3);
  EXPECT_EQ(TH1.getTransitivelyReachableTypes("struct.Child").size(), 2);
  EXPECT_EQ(TH1.getTransitivelyReachableTypes("struct.ChildsChild").size(), 1);
  auto BaseReachable = TH1.getTransitivelyReachableTypes("class.Base");
  EXPECT_TRUE(BaseReachable.count("class.Base"));
  EXPECT_TRUE(BaseReachable.count("struct.Child"));
  EXPECT_TRUE(BaseReachable.count("struct.ChildsChild"));
  auto ChildReachable = TH1.getTransitivelyReachableTypes("struct.Child");
  EXPECT_TRUE(ChildReachable.count("struct.Child"));
  EXPECT_TRUE(ChildReachable.count("struct.ChildsChild"));
  auto ChildsChildReachable =
      TH1.getTransitivelyReachableTypes("struct.ChildsChild");
  EXPECT_TRUE(ChildsChildReachable.count("struct.ChildsChild"));
}

TEST_F(LTHTest, HandleSTLString) {
  ProjectIRDB IRDB(
      {pathToLLFiles + "type_hierarchies/type_hierarchy_13_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  EXPECT_EQ(TH.getNumTypes(), 4);
  EXPECT_TRUE(TH.containsType("class.std::__cxx11::basic_string"));
  EXPECT_TRUE(TH.containsType("struct.std::__cxx11::basic_string<char, "
                              "std::char_traits<char>, std::allocator<char> "
                              ">::_Alloc_hider"));
  EXPECT_TRUE(TH.containsType("union.anon"));
  EXPECT_TRUE(TH.containsType("class.std::allocator"));
  EXPECT_TRUE(TH.hasSubType("struct.std::__cxx11::basic_string<char, "
                            "std::char_traits<char>, std::allocator<char> "
                            ">::_Alloc_hider",
                            "class.std::__cxx11::basic_string"));
  EXPECT_TRUE(TH.hasSubType("union.anon", "class.std::__cxx11::basic_string"));
  EXPECT_TRUE(TH.hasSuperType("class.std::__cxx11::basic_string",
                              "struct.std::__cxx11::basic_string<char, "
                              "std::char_traits<char>, std::allocator<char> "
                              ">::_Alloc_hider"));
  EXPECT_TRUE(TH.hasSuperType("class.std::allocator", "class.std::allocator"));
}

} // namespace psr

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto res = RUN_ALL_TESTS();
  llvm::llvm_shutdown();
  return res;
}
