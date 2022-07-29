#include "gtest/gtest.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardICFG.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

#include "llvm/IR/InstIterator.h"

using namespace std;
using namespace psr;

class LLVMBasedBackwardICFGTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      PhasarConfig::PhasarDirectory() + "build/test/llvm_test_code/";
};

TEST_F(LLVMBasedBackwardICFGTest, test1) {
  // TODO add suitable test cases
  // ASSERT_FALSE(true);
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
