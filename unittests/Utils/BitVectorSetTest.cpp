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

TEST(BitVectorSet, insertBitVectorSet) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B;
  BitVectorSet<int> C({1, 2, 3, 4, 5, 6});
  A.insert(B);
  EXPECT_EQ(A, C);

  BitVectorSet<int> D({1, 2, 42});
  BitVectorSet<int> E({1, 2, 3, 4, 5, 6, 42});
  A.insert(D);
  EXPECT_EQ(A, E);

  BitVectorSet<int> F({1, 2, 3, 4, 5, 6, 7});
  BitVectorSet<int> G;
  G.insert(F);
  EXPECT_EQ(F, G);

  BitVectorSet<int> H;
  BitVectorSet<int> I;
  H.insert(I);
  EXPECT_EQ(H, I);
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

TEST(BitVectorSet, setUnion) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B({5, 6, 42});

  BitVectorSet<int> C({1, 2, 3, 4, 5, 6, 42});
  BitVectorSet<int> D;

  A.setUnion(B);

  EXPECT_TRUE(A == C);
  A.setUnion(D);
  EXPECT_TRUE(A == C);
}

TEST(BitVectorSet, setIntersect) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B({5, 6, 42});

  BitVectorSet<int> C({1, 2, 3, 4, 5, 6, 42});
  BitVectorSet<int> D({5, 6});

  BitVectorSet<int> A2 = A.setIntersect(B);

  EXPECT_TRUE(A2 != C);
  EXPECT_TRUE(A2 == D);
  BitVectorSet<int> A3 = A2.setIntersect(D);
  EXPECT_TRUE(A3 == D);
  BitVectorSet<int> A4 = A3.setIntersect(BitVectorSet<int>());
  EXPECT_TRUE(A4.empty());
}

TEST(BitVectorSet, includes) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B({1, 2, 3});
  BitVectorSet<int> C({1, 2, 42});
  BitVectorSet<int> D({1, 2, 3, 4, 5, 6, 7});
  BitVectorSet<int> E({1, 2, 3});

  EXPECT_TRUE(A.includes(B));
  EXPECT_FALSE(B.includes(A));
  EXPECT_FALSE(A.includes(C));
  EXPECT_FALSE(B.includes(C));
  EXPECT_FALSE(A.includes(D));
  EXPECT_TRUE(D.includes(A));
  EXPECT_TRUE(B.includes(E));
  EXPECT_TRUE(E.includes(B));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
