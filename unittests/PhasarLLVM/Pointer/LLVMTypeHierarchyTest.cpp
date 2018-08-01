#include <boost/graph/isomorphism.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>

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

TEST_F(LTHTest, GraphConstruction) {
  ProjectIRDB IRDB({pathToLLFiles + "type_hierarchies/type_hierarchy_1.ll",
                    pathToLLFiles + "type_hierarchies/type_hierarchy_7.ll",
                    pathToLLFiles + "type_hierarchies/type_hierarchy_8.ll",
                    pathToLLFiles + "type_hierarchies/type_hierarchy_9.ll",
                    pathToLLFiles + "type_hierarchies/type_hierarchy_10.ll"});
  llvm::Module *M =
      IRDB.getModule(pathToLLFiles + "type_hierarchies/type_hierarchy_1.ll");
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH1;
  TH1.analyzeModule(*M);

  LLVMTypeHierarchy::bidigraph_t graph1;
  auto base_vertex_1 = boost::add_vertex(graph1);
  auto child_vertex_1 = boost::add_vertex(graph1);
  boost::add_edge(child_vertex_1, base_vertex_1, graph1);

  ASSERT_TRUE(boost::isomorphism(graph1, TH1.g));

  M = IRDB.getModule(pathToLLFiles + "type_hierarchies/type_hierarchy_7.ll");
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH2;
  TH2.analyzeModule(*M);

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

  M = IRDB.getModule(pathToLLFiles + "type_hierarchies/type_hierarchy_8.ll");
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH3;
  TH3.analyzeModule(*M);

  LLVMTypeHierarchy::bidigraph_t graph3;
  auto base_vertex_3 = boost::add_vertex(graph3);
  auto child_vertex_3 = boost::add_vertex(graph3);
  auto NonvirtualClass_vertex_3 = boost::add_vertex(graph3);
  auto NonvirtualStruct_vertex_3 = boost::add_vertex(graph3);

  boost::add_edge(child_vertex_3, base_vertex_3, graph3);

  ASSERT_TRUE(boost::isomorphism(graph3, TH3.g));

  M = IRDB.getModule(pathToLLFiles + "type_hierarchies/type_hierarchy_9.ll");
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH4;
  TH4.analyzeModule(*M);

  LLVMTypeHierarchy::bidigraph_t graph4;
  auto base_vertex_4 = boost::add_vertex(graph4);
  auto child_vertex_4 = boost::add_vertex(graph4);

  boost::add_edge(child_vertex_4, base_vertex_4, graph4);

  ASSERT_TRUE(boost::isomorphism(graph4, TH4.g));

  M = IRDB.getModule(pathToLLFiles + "type_hierarchies/type_hierarchy_10.ll");
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH5;
  TH5.analyzeModule(*M);

  LLVMTypeHierarchy::bidigraph_t graph5;
  auto base_vertex_5 = boost::add_vertex(graph5);
  auto child_vertex_5 = boost::add_vertex(graph5);

  boost::add_edge(child_vertex_5, base_vertex_5, graph5);

  ASSERT_TRUE(boost::isomorphism(graph5, TH5.g));
}

#include <iostream>

TEST_F(LTHTest, VTableConstruction) {
  ProjectIRDB IRDB1({pathToLLFiles + "type_hierarchies/type_hierarchy_1.ll"});
  ProjectIRDB IRDB2({pathToLLFiles + "type_hierarchies/type_hierarchy_7.ll"});
  ProjectIRDB IRDB3({pathToLLFiles + "type_hierarchies/type_hierarchy_8.ll"});
  ProjectIRDB IRDB4({pathToLLFiles + "type_hierarchies/type_hierarchy_9.ll"});
  ProjectIRDB IRDB5({pathToLLFiles + "type_hierarchies/type_hierarchy_10.ll"});

  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH1(IRDB1);
  LLVMTypeHierarchy TH2(IRDB2);
  LLVMTypeHierarchy TH3(IRDB3);
  LLVMTypeHierarchy TH4(IRDB4);
  LLVMTypeHierarchy TH5(IRDB5);

  ASSERT_TRUE(TH1.containsVTable("struct.Base"));
  ASSERT_TRUE(TH1.containsVTable("struct.Base.base"));
  ASSERT_TRUE(TH1.containsVTable("class.Base"));
  ASSERT_TRUE(TH1.containsVTable("Base"));
  ASSERT_TRUE(TH1.containsVTable("struct.Child"));
  ASSERT_TRUE(TH1.containsVTable("struct.Child.base"));
  ASSERT_TRUE(TH1.containsVTable("class.Child"));
  ASSERT_TRUE(TH1.containsVTable("Child"));
  ASSERT_FALSE(TH1.containsVTable("struct.ANYTHING"));

  ASSERT_TRUE(TH2.containsVTable("A"));
  ASSERT_TRUE(TH2.containsVTable("B"));
  ASSERT_TRUE(TH2.containsVTable("C"));
  ASSERT_TRUE(TH2.containsVTable("D"));
  ASSERT_TRUE(TH2.containsVTable("X"));
  ASSERT_TRUE(TH2.containsVTable("Y"));
  ASSERT_TRUE(TH2.containsVTable("Z"));

  ASSERT_TRUE(TH3.containsVTable("Base"));
  ASSERT_TRUE(TH3.containsVTable("Child"));
  ASSERT_FALSE(TH3.containsVTable("NonvirtualClass"));
  ASSERT_FALSE(TH3.containsVTable("NonvirtualStruct"));

  ASSERT_TRUE(TH4.containsVTable("Base"));
  ASSERT_TRUE(TH4.containsVTable("Child"));

  ASSERT_TRUE(TH5.containsVTable("Base"));
  ASSERT_TRUE(TH5.containsVTable("Child"));

  ASSERT_TRUE(cxx_demangle(TH1.getVTableEntry("Base", 0)) == "Base::foo()");
  ASSERT_TRUE(cxx_demangle(TH1.getVTableEntry("Base", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH1.getVTableEntry("Child", 0)) == "Child::foo()");
  ASSERT_TRUE(cxx_demangle(TH1.getVTableEntry("Child", 1)) == "");

  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("A", 0)) == "A::f()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("A", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("B", 0)) == "A::f()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("B", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("C", 0)) == "A::f()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("C", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("D", 0)) == "A::f()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("D", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("X", 0)) == "X::g()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("X", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("Y", 0)) == "X::g()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("Y", 1)) == "");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("Z", 0)) == "A::f()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("Z", 1)) == "X::g()");
  ASSERT_TRUE(cxx_demangle(TH2.getVTableEntry("Z", 2)) == "");

  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("Base", 0)) == "Base::foo()");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("Base", 1)) == "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("Base", 2)) == "");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("Child", 0)) == "Child::foo()");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("Child", 1)) == "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("Child", 2)) == "Child::baz()");
  ASSERT_TRUE(cxx_demangle(TH3.getVTableEntry("Child", 3)) == "");

  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("Base", 0)) == "Base::foo()");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("Base", 1)) == "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("Base", 2)) == "");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("Child", 0)) == "Child::foo()");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("Child", 1)) == "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("Child", 2)) == "Child::baz()");
  ASSERT_TRUE(cxx_demangle(TH4.getVTableEntry("Child", 3)) == "");

  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("Base", 0)) ==
              "__cxa_pure_virtual");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("Base", 1)) == "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("Base", 2)) == "");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("Child", 0)) == "Child::foo()");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("Child", 1)) == "Base::bar()");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("Child", 2)) == "Child::baz()");
  ASSERT_TRUE(cxx_demangle(TH5.getVTableEntry("Child", 3)) == "");
}

TEST_F(LTHTest, TransitivelyReachableTypes) {
  ProjectIRDB IRDB1({pathToLLFiles + "type_hierarchies/type_hierarchy_1.ll"});
  ProjectIRDB IRDB2({pathToLLFiles + "type_hierarchies/type_hierarchy_7.ll"});
  ProjectIRDB IRDB3({pathToLLFiles + "type_hierarchies/type_hierarchy_8.ll"});
  ProjectIRDB IRDB4({pathToLLFiles + "type_hierarchies/type_hierarchy_9.ll"});
  ProjectIRDB IRDB5({pathToLLFiles + "type_hierarchies/type_hierarchy_10.ll"});
  // Creates an empty type hierarchy
  LLVMTypeHierarchy TH1(IRDB1);
  LLVMTypeHierarchy TH2(IRDB2);
  LLVMTypeHierarchy TH3(IRDB3);
  LLVMTypeHierarchy TH4(IRDB4);
  LLVMTypeHierarchy TH5(IRDB5);

  auto reachable_types_base_1 = TH1.getTransitivelyReachableTypes("Base");
  auto reachable_types_child_1 = TH1.getTransitivelyReachableTypes("Child");

  auto reachable_types_A_2 = TH2.getTransitivelyReachableTypes("A");
  auto reachable_types_B_2 = TH2.getTransitivelyReachableTypes("B");
  auto reachable_types_C_2 = TH2.getTransitivelyReachableTypes("C");
  auto reachable_types_D_2 = TH2.getTransitivelyReachableTypes("D");
  auto reachable_types_X_2 = TH2.getTransitivelyReachableTypes("X");
  auto reachable_types_Y_2 = TH2.getTransitivelyReachableTypes("Y");
  auto reachable_types_Z_2 = TH2.getTransitivelyReachableTypes("Z");

  auto reachable_types_base_3 = TH3.getTransitivelyReachableTypes("Base");
  auto reachable_types_child_3 = TH3.getTransitivelyReachableTypes("Child");
  auto reachable_types_nonvirtualclass_3 =
      TH3.getTransitivelyReachableTypes("NonvirtualClass");
  auto reachable_types_nonvirtualstruct_3 =
      TH3.getTransitivelyReachableTypes("NonvirtualStruct");

  auto reachable_types_base_4 = TH4.getTransitivelyReachableTypes("Base");
  auto reachable_types_child_4 = TH4.getTransitivelyReachableTypes("Child");

  auto reachable_types_base_5 = TH5.getTransitivelyReachableTypes("Base");
  auto reachable_types_child_5 = TH5.getTransitivelyReachableTypes("Child");

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
  ProjectIRDB IRDB({pathToLLFiles + "type_hierarchies/type_hierarchy_1.ll"});
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
  ProjectIRDB IRDB({pathToLLFiles + "taint_analysis/growing_example.ll"});
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

TEST_F(LTHTest, SameTypeDifferentVTables) {
  ProjectIRDB IRDB1({pathToLLFiles + "module_wise/module_wise_9/src1.ll"});
  LLVMTypeHierarchy TH1(IRDB1);
  TH1.print();
  ProjectIRDB IRDB2({pathToLLFiles + "module_wise/module_wise_9/src2.ll"});
  LLVMTypeHierarchy TH2(IRDB2);
  TH2.print();
}
} // namespace psr

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto res = RUN_ALL_TESTS();
  llvm::llvm_shutdown();
  return res;
}
