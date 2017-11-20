#include <gtest/gtest.h>
#include <vector>
using namespace std;

TEST (FirstTest1, FirstTestName1) {
	EXPECT_EQ (18.0, 18.0);
}

TEST (FirstTest2, FirstOtherTestName2) {
	vector<int> iv = { 1, 2, 3 };
	ASSERT_EQ (3, iv.size());
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}