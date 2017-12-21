#include "../../src/db/DBConn.h"
#include "../../src/db/ProjectIRDB.h"
//#include <thread>
//#include "../../src/analysis/points-to/LLVMTypeHierarchy.h"
//#include <boost/graph/adjacency_list.hpp>
//#include <boost/graph/graph_utility.hpp>
//#include <boost/graph/graphviz.hpp>
//#include <boost/property_map/dynamic_property_map.hpp>
#include <gtest/gtest.h>
using namespace std;

TEST(StoreLLVMTypeHierarchyTest, HandleMultipleModuleLTH) {
  ProjectIRDB IRDB(
    {"test_code/llvm_test_code/module_wise/module_wise_9/src1.ll",
     "test_code/llvm_test_code/module_wise/module_wise_9/src2.ll",
     "test_code/llvm_test_code/module_wise/module_wise_9/src3.ll"});
  LLVMTypeHierarchy TH(IRDB);
  TH.print();
  cout << '\n';
  DBConn &db = DBConn::getInstance();
  db.storeProjectIRDB("phasardbtest", IRDB);
//  this_thread::sleep_for(10s);
  db.storeLLVMTypeHierarchy(TH, false);
//  db.dropDBAndRebuildScheme();
//  db.storeLLVMTypeHierarchy(TH, true);
}

TEST(StoreProjectIRDBTest, StoreProjectIRDBTest) {
  ProjectIRDB IRDB(
      {"test_code/llvm_test_code/module_wise/module_wise_9/src1.ll",
       "test_code/llvm_test_code/module_wise/module_wise_9/src2.ll",
       "test_code/llvm_test_code/module_wise/module_wise_9/src3.ll"});
  DBConn &db = DBConn::getInstance();
  db.storeProjectIRDB("phasardbtest", IRDB);
  cout << db.getFunctionHash(10) << endl;
  cout << db.getFunctionHash(1) << endl;
  cout << db.getFunctionHash(3) << endl;
//  for (auto g : db.getGlobalVariableID("_ZTV13OtherConcrete")) {
//    cout << g << ":" << db.globalVariableIsDeclaration(g) << endl;
//  }
//  cout << std::boolalpha << db.globalVariableIsDeclaration(1) << endl;
//  cout << std::boolalpha << db.globalVariableIsDeclaration(2) << endl;
//  cout << std::boolalpha << db.globalVariableIsDeclaration(3) << endl;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}