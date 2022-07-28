#include "gtest/gtest.h"

#include "llvm/IR/InstIterator.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardICFG.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace std;
using namespace psr;

class LLVMBasedBackwardICFGTest : public ::testing::Test {};

TEST_F(LLVMBasedBackwardICFGTest, test1) {
  // TODO add suitable test cases
  // ASSERT_FALSE(true);
}
