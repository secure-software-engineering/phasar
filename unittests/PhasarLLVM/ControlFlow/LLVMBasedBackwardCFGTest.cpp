#include <gtest/gtest.h>
#include <llvm/IR/InstIterator.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

class LLVMBasedBackwardCFGTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedBackwardCFGTest, FallThroughSuccTest) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunction("main");

  ASSERT_EQ(true,false);

}

TEST_F(LLVMBasedBackwardCFGTest, BranchTargetTest) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/switch_cpp.ll"});
  auto F = IRDB.getFunction("main");

  ASSERT_EQ(true,false);
}

TEST_F(LLVMBasedBackwardCFGTest, HandlesMulitplePredeccessors) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunction("main");

  ASSERT_EQ(true,false);

}

TEST_F(LLVMBasedBackwardCFGTest, HandlesSingleOrEmptyPredeccessor) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunction("main");

  ASSERT_EQ(true,false);

  
}

TEST_F(LLVMBasedBackwardCFGTest, HandlesMultipleSuccessors) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/branch_cpp.ll"});
  auto F = IRDB.getFunction("main");

  ASSERT_EQ(true,false);

}

TEST_F(LLVMBasedBackwardCFGTest, HandlesSingleOrEmptySuccessor) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/function_call_cpp.ll"});
  auto F = IRDB.getFunction("main");

  ASSERT_EQ(true,false);

}

TEST_F(LLVMBasedBackwardCFGTest, HandlesCallSuccessor) {
  LLVMBasedBackwardCFG cfg;
  ProjectIRDB IRDB({pathToLLFiles + "control_flow/function_call_cpp.ll"});
  auto F = IRDB.getFunction("main");

  ASSERT_EQ(true,false);

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
