/*
#include "phasar/DB/DBConn.h"
#include "phasar/DB/ProjectIRDB.h"
#include "gtest/gtest.h"

using namespace std;
using namespace psr;

class DBConnTest : public ::testing::Test {
protected:
  const string pathToLLFiles = PhasarDirectory + "build/test/llvm_test_code/";
};

TEST_F(DBConnTest, HandleLTHStoreWithMultipleProjects) {
  ProjectIRDB firstIRDB(
      {pathToLLFiles + "module_wise/module_wise_13/src1_cpp.ll",
       pathToLLFiles + "module_wise/module_wise_13/src2_cpp.ll",
       pathToLLFiles + "module_wise/module_wise_13/main_cpp.ll"});
  ProjectIRDB secondIRDB(
      {pathToLLFiles + "module_wise/module_wise_14/src1_cpp.ll",
       pathToLLFiles + "module_wise/module_wise_14/src2_cpp.ll",
       pathToLLFiles + "module_wise/module_wise_14/main_cpp.ll"});

  DBConn &db = DBConn::getInstance();
  LLVMTypeHierarchy TH1(firstIRDB);
  LLVMTypeHierarchy TH2(secondIRDB);
  cout << "\n\n";
  TH1.print();
  cout << "\n\n";
  TH2.print();
  db.storeProjectIRDB("first_project", firstIRDB);
  db.storeProjectIRDB("second_project", secondIRDB);
}

// TEST_F(DBConnTest, HandleLTHWriteToHex) {
//   ProjectIRDB IRDB({pathToLLFiles + "module_wise/module_wise_9/src1_cpp.ll",
//                     pathToLLFiles + "module_wise/module_wise_9/src2_cpp.ll",
//                     pathToLLFiles +
//                     "module_wise/module_wise_9/src3_cpp.ll"});
//   DBConn &db = DBConn::getInstance();
//   db.storeProjectIRDB("phasardbtest", IRDB);
//   LLVMTypeHierarchy TH(IRDB);
//   cout << "\n\n";
//   TH.print();
//   cout << '\n';
//   db.storeLLVMTypeHierarchy(TH, "phasardbtest", true);
// }

TEST_F(DBConnTest, HandleLTHWriteToDot) {
  ProjectIRDB IRDB({pathToLLFiles + "module_wise/module_wise_9/src1_cpp.ll",
                    pathToLLFiles + "module_wise/module_wise_9/src2_cpp.ll",
                    pathToLLFiles + "module_wise/module_wise_9/src3_cpp.ll"});
  DBConn &db = DBConn::getInstance();
  db.storeProjectIRDB("phasardbtest", IRDB);
  LLVMTypeHierarchy TH(IRDB);
  cout << '\n';
  TH.print();
  cout << '\n';
  db.storeLLVMTypeHierarchy(TH, "phasardbtest", false);
}

TEST_F(DBConnTest, StoreProjectIRDBTest) {
  ProjectIRDB IRDB({pathToLLFiles + "module_wise/module_wise_9/src1_cpp.ll",
                    pathToLLFiles + "module_wise/module_wise_9/src2_cpp.ll",
                    pathToLLFiles + "module_wise/module_wise_9/src3_cpp.ll"});
  DBConn &db = DBConn::getInstance();
  db.storeProjectIRDB("phasardbtest", IRDB);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
*/