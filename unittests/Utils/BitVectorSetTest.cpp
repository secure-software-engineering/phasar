#include <gtest/gtest.h>

#include <iostream>

#include <phasar/Utils/BitVectorSet.h>

using namespace psr;

TEST(BitVectorSet, ctor) {
  BitVectorSet<int> B({10, 20, 30, 40, 50});

  EXPECT_EQ(B.count(10), 1);
  EXPECT_EQ(B.count(20), 1);
  EXPECT_EQ(B.count(30), 1);
  EXPECT_EQ(B.count(40), 1);
  EXPECT_EQ(B.count(50), 1);
  EXPECT_EQ(B.count(666), 0);
}

TEST(BitVectorSetTest, insert) {
  BitVectorSet<int> B;
  B.insert(1);
  B.insert(42);
  B.insert(13);

  EXPECT_EQ(B.count(1), 1);
  EXPECT_EQ(B.count(42), 1);
  EXPECT_EQ(B.count(13), 1);
  EXPECT_EQ(B.count(666), 0);
}

TEST(BitVectorSet, twoSets) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B({5, 6, 42});

  EXPECT_EQ(A.count(1), 1);
  EXPECT_EQ(A.count(2), 1);
  EXPECT_EQ(A.count(3), 1);
  EXPECT_EQ(A.count(4), 1);
  EXPECT_EQ(A.count(5), 1);
  EXPECT_EQ(A.count(6), 1);
  EXPECT_EQ(A.count(42), 0);

  EXPECT_EQ(B.count(5), 1);
  EXPECT_EQ(B.count(6), 1);
  EXPECT_EQ(B.count(42), 1);
  EXPECT_EQ(B.count(1), 0);
  EXPECT_EQ(B.count(2), 0);
  EXPECT_EQ(B.count(3), 0);
}

TEST(BitVectorSet, equality) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B({5, 6, 42});
  BitVectorSet<int> C({5, 6, 42});
  BitVectorSet<int> D({5, 6, 42, 13});
  BitVectorSet<int> E({1, 2, 3});

  EXPECT_EQ((A == B), false);
  EXPECT_EQ((A != B), true);

  EXPECT_EQ((B == C), true);
  EXPECT_EQ((B != C), false);

  EXPECT_EQ((C == D), false);
  EXPECT_EQ((D == C), false);

  EXPECT_EQ((A == E), false);
  EXPECT_EQ((A != E), true);

  A.clear();
  A.insert(1);
  A.insert(2);
  A.insert(3);
  EXPECT_EQ((A == E), 1);
}

TEST(BitVectorSet, size) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B({5, 6, 42});
  BitVectorSet<int> C({5, 6, 42});
  BitVectorSet<int> D({5, 6, 42, 13});
  BitVectorSet<int> E({1, 2, 3});
  BitVectorSet<int> F;

  EXPECT_EQ(A.size(), 6);
  EXPECT_EQ(B.size(), 3);
  EXPECT_EQ(C.size(), 3);
  EXPECT_EQ(D.size(), 4);
  EXPECT_EQ(E.size(), 3);
  EXPECT_EQ(F.size(), 0);
}

TEST(BitVectorSet, empty) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B({5, 6, 42});
  BitVectorSet<int> C({5, 6, 42});
  BitVectorSet<int> D({5, 6, 42, 13});
  BitVectorSet<int> E({1, 2, 3});
  BitVectorSet<int> F;

  EXPECT_EQ(A.empty(), false);
  EXPECT_EQ(B.empty(), false);
  EXPECT_EQ(C.empty(), false);
  EXPECT_EQ(D.empty(), false);
  EXPECT_EQ(E.empty(), false);
  EXPECT_EQ(F.empty(), true);
}

TEST(BitVectorSet, clear) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B({5, 6, 42});
  BitVectorSet<int> C;

  EXPECT_EQ(A.size(), 6);
  A.clear();
  EXPECT_EQ(A.size(), 0);
  B.clear();
  EXPECT_EQ(B.size(), 0);
  C.clear();
  EXPECT_EQ(C.size(), 0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
