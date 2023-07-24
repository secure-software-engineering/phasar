#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardICFG.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/InstIterator.h"

#include "gtest/gtest.h"

using namespace std;
using namespace psr;

class LLVMBasedBackwardICFGTest : public ::testing::Test {
protected:
};

TEST_F(LLVMBasedBackwardICFGTest, test1) {
  // TODO add suitable test cases
  // ASSERT_FALSE(true);
}
