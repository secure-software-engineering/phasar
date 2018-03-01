#include <phasar/DB/DBConn.h>
#include <phasar/DB/ProjectIRDB.h>
#include <gtest/gtest.h>

const std::vector<std::vector<std::string>> IRFiles{
    /* IRFiles[0] */
    {"test_code/llvm_test_code/virtual_callsites/cross_module/base.ll"},
    /* IRFiles[1] */
    {"test_code/llvm_test_code/virtual_callsites/cross_module/main.ll",
     "test_code/llvm_test_code/virtual_callsites/cross_module/utils.ll",
     "test_code/llvm_test_code/virtual_callsites/cross_module/base.ll",
     "test_code/llvm_test_code/virtual_callsites/cross_module/derived.ll"},
    /* IRFiles[2] */
    {"test_code/llvm_test_code/module_wise/module_wise_9/src1.ll",
     "test_code/llvm_test_code/module_wise/module_wise_9/src2.ll",
     "test_code/llvm_test_code/module_wise/module_wise_9/src3.ll"},
    /* IRFiles[3] */
    {"test_code/llvm_test_code/module_wise/module_wise_12/src1.ll",
     "test_code/llvm_test_code/module_wise/module_wise_12/src2.ll",
     "test_code/llvm_test_code/module_wise/module_wise_12/main.ll"},
    /* IRFiles[4] */
    {"test_code/llvm_test_code/module_wise/module_wise_13/src1.ll",
     "test_code/llvm_test_code/module_wise/module_wise_13/src2.ll",
     "test_code/llvm_test_code/module_wise/module_wise_13/main.ll"},
    /* IRFiles[5] */
    {"test_code/llvm_test_code/module_wise/module_wise_14/src1.ll",
     "test_code/llvm_test_code/module_wise/module_wise_14/src2.ll",
     "test_code/llvm_test_code/module_wise/module_wise_14/main.ll"}};

TEST(StoreLLVMTypeHierarchyTest, HandleMultipleProjects) {
  ProjectIRDB firstIRDB(IRFiles[4]);
  ProjectIRDB secondIRDB(IRFiles[5]);

  DBConn &db = DBConn::getInstance();
  LLVMTypeHierarchy TH1(firstIRDB);
  LLVMTypeHierarchy TH2(secondIRDB);
  std::cout << "\n\n";
  TH1.print();
  std::cout << "\n\n";
  TH2.print();
  db.storeProjectIRDB("first_project", firstIRDB);
  db.storeProjectIRDB("second_project", secondIRDB);
}

TEST(StoreLLVMTypeHierarchyTest, HandleWriteToHex) {
  ProjectIRDB IRDB(IRFiles[2]);
  DBConn &db = DBConn::getInstance();
  db.storeProjectIRDB("phasardbtest", IRDB);
  LLVMTypeHierarchy TH(IRDB);
  std::cout << "\n\n";
  TH.print();
  std::cout << '\n';
  db.storeLLVMTypeHierarchy(TH, "phasardbtest", true);
}

TEST(StoreLLVMTypeHierarchyTest, HandleWriteToDot) {
  ProjectIRDB IRDB(IRFiles[2]);
  DBConn &db = DBConn::getInstance();
  db.storeProjectIRDB("phasardbtest", IRDB);
  LLVMTypeHierarchy TH(IRDB);
  std::cout << '\n';
  TH.print();
  std::cout << '\n';
  db.storeLLVMTypeHierarchy(TH, "phasardbtest", false);
}

TEST(StoreProjectIRDBTest, StoreProjectIRDBTest) {
  ProjectIRDB IRDB(IRFiles[2]);
  DBConn &db = DBConn::getInstance();
  db.storeProjectIRDB("phasardbtest", IRDB);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}