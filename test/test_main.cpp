#include <gtest/gtest.h>
#include <vector>
using namespace std;

TEST (SimpleTest, TestName) {
	EXPECT_EQ (18.0, 18.0);
}

TEST (OtherTest, OtherTestName) {
	vector<int> iv = { 1, 2, 3 };
	ASSERT_EQ (3, iv.size());
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}