#include <gtest/gtest.h>
#include <llvm/IR/InstIterator.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardICFG.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>

using namespace std;
using namespace psr;

class LLVMBasedBackwardICFGTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarDirectory + "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedBackwardICFGTest, test1)
{
    ASSERT_FALSE(true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}