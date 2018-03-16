#include "../../../src/analysis/points-to/LLVMTypeHierarchy.h"
#include "../../../src/db/ProjectIRDB.h"
#include <gtest/gtest.h>

// TODO: figure out how to automatically compare graphs for these tests
TEST(LTHGraphDotTest, HandleLoadAndPrintOfNonEmptyGraph) {
  ProjectIRDB IRDB(
    {"test_code/llvm_test_code/type_hierarchies/type_hierarchy_1.ll"});
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
//  ASSERT_EQ(oss.str(),oss2.str());
}

TEST(LTHGraphDotTest, HandleLoadAndPrintOfEmptyGraph) {
  ProjectIRDB IRDB(
      {"test_code/llvm_test_code/taint_analysis/growing_example.ll"});
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
  ASSERT_EQ(oss.str(),oss2.str());
}

TEST(VTableTest, SameTypeDifferentVTables) {
  ProjectIRDB IRDB1(
    {"test_code/llvm_test_code/module_wise/module_wise_9/src1.ll"});
  LLVMTypeHierarchy TH1(IRDB1);
  TH1.print();
  ProjectIRDB IRDB2(
    {"test_code/llvm_test_code/module_wise/module_wise_9/src2.ll"});
  LLVMTypeHierarchy TH2(IRDB2);
  TH2.print();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}