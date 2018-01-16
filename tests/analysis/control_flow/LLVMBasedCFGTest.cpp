#include <gtest/gtest.h>
#include "../../../src/analysis/control_flow/LLVMBasedCFG.h"
using namespace std;

TEST (GetSuccsTest, HandlesSingleSuccessor) {
	ASSERT_EQ (18.0, 18.0);
}

TEST (GetSuccsTest, HandlesMultipleSuccessors) {
	ASSERT_EQ (3, 3);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}