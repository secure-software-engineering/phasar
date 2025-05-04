#include "phasar/Utils/BitVectorSet.h"

#include "phasar/Utils/DebugOutput.h"

#include "llvm/ADT/BitVector.h"

#include "gtest/gtest.h"

#include <set>
#include <unordered_set>
#include <utility>

using namespace psr;
using namespace std;

TEST(BitVectorSet, ctor) {
  BitVectorSet<int> B({10, 20, 30, 40, 50});

  EXPECT_EQ(B.count(10), 1U);
  EXPECT_EQ(B.count(20), 1U);
  EXPECT_EQ(B.count(30), 1U);
  EXPECT_EQ(B.count(40), 1U);
  EXPECT_EQ(B.count(50), 1U);
  EXPECT_EQ(B.count(666), 0U);
}

TEST(BitVectorSet, ctorIter) {
  std::set<int> S({10, 20, 30, 40, 50});
  BitVectorSet<int> B(S.begin(), S.end());

  EXPECT_EQ(B.count(10), 1U);
  EXPECT_EQ(B.count(20), 1U);
  EXPECT_EQ(B.count(30), 1U);
  EXPECT_EQ(B.count(40), 1U);
  EXPECT_EQ(B.count(50), 1U);
  EXPECT_EQ(B.count(666), 0U);
}

TEST(BitVectorSet, copy) {
  BitVectorSet<int> B({10, 20, 30, 40, 50});
  BitVectorSet<int> C(B);

  EXPECT_TRUE(B == C);

  B.insert(1);
  B.insert(42);
  B.insert(13);

  EXPECT_EQ(C.count(10), 1U);
  EXPECT_EQ(C.count(20), 1U);
  EXPECT_EQ(C.count(30), 1U);
  EXPECT_EQ(C.count(40), 1U);
  EXPECT_EQ(C.count(50), 1U);
  EXPECT_EQ(C.count(666), 0U);
}

TEST(BitVectorSet, copyAssign) {
  BitVectorSet<int> B({10, 20, 30, 40, 50});
  BitVectorSet<int> C;
  C = B;

  EXPECT_TRUE(B == C);

  B.insert(1);
  B.insert(42);
  B.insert(13);

  EXPECT_EQ(C.count(10), 1U);
  EXPECT_EQ(C.count(20), 1U);
  EXPECT_EQ(C.count(30), 1U);
  EXPECT_EQ(C.count(40), 1U);
  EXPECT_EQ(C.count(50), 1U);
  EXPECT_EQ(C.count(666), 0U);
}

TEST(BitVectorSet, move) {
  BitVectorSet<int> B({10, 20, 30, 40, 50});
  BitVectorSet<int> C(B);

  EXPECT_EQ(C.count(10), 1U);
  EXPECT_EQ(C.count(20), 1U);
  EXPECT_EQ(C.count(30), 1U);
  EXPECT_EQ(C.count(40), 1U);
  EXPECT_EQ(C.count(50), 1U);
  EXPECT_EQ(C.count(666), 0U);

  C.insert(42);

  EXPECT_EQ(C.count(42), 1U);
}

TEST(BitVectorSet, insert) {
  BitVectorSet<int> B;
  B.insert(1);
  B.insert(42);
  B.insert(13);

  EXPECT_EQ(B.count(1), 1U);
  EXPECT_EQ(B.count(42), 1U);
  EXPECT_EQ(B.count(13), 1U);
  EXPECT_EQ(B.count(666), 0U);
}

TEST(BitVectorSet, insertIter) {
  std::set<int> S = {1, 42, 13};
  BitVectorSet<int> B;
  B.insert(S.begin(), S.end());

  EXPECT_EQ(B.count(1), 1U);
  EXPECT_EQ(B.count(42), 1U);
  EXPECT_EQ(B.count(13), 1U);
  EXPECT_EQ(B.count(666), 0U);
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

  EXPECT_EQ(A.count(1), 1U);
  EXPECT_EQ(A.count(2), 1U);
  EXPECT_EQ(A.count(3), 1U);
  EXPECT_EQ(A.count(4), 1U);
  EXPECT_EQ(A.count(5), 1U);
  EXPECT_EQ(A.count(6), 1U);
  EXPECT_EQ(A.count(42), 0U);

  EXPECT_EQ(B.count(5), 1U);
  EXPECT_EQ(B.count(6), 1U);
  EXPECT_EQ(B.count(42), 1U);
  EXPECT_EQ(B.count(1), 0U);
  EXPECT_EQ(B.count(2), 0U);
  EXPECT_EQ(B.count(3), 0U);
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
  EXPECT_EQ((A == E), true);
}

TEST(BitVectorSet, size) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B({5, 6, 42});
  BitVectorSet<int> C({5, 6, 42});
  BitVectorSet<int> D({5, 6, 42, 13});
  BitVectorSet<int> E({1, 2, 3});
  BitVectorSet<int> F;

  EXPECT_EQ(A.size(), 6U);
  EXPECT_EQ(B.size(), 3U);
  EXPECT_EQ(C.size(), 3U);
  EXPECT_EQ(D.size(), 4U);
  EXPECT_EQ(E.size(), 3U);
  EXPECT_EQ(F.size(), 0U);
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

  EXPECT_EQ(A.size(), 6U);
  A.clear();
  EXPECT_EQ(A.size(), 0U);
  B.clear();
  EXPECT_EQ(B.size(), 0U);
  C.clear();
  EXPECT_EQ(C.size(), 0U);
}

TEST(BitVectorSet, setUnion) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B({5, 6, 42});

  BitVectorSet<int> C({1, 2, 3, 4, 5, 6, 42});
  BitVectorSet<int> D;

  A = A.setUnion(B);

  EXPECT_TRUE(A == C);
  A = A.setUnion(D);
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

namespace std {

template <> struct hash<pair<int, int>> {
  size_t operator()(const pair<int, int> &P) const {
    return std::hash<int>()(P.first + P.second);
  }
};

} // namespace std

TEST(BitVectorSet, includesForIntegers) {

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

  BitVectorSet<int> X({1, 2, 3, 4, 5, 6, 7});
  BitVectorSet<int> Y({7});

  EXPECT_TRUE(X.includes(Y));
}

TEST(BitVectorSet, includesForPairsOfIntegers) {
  BitVectorSet<std::pair<int, int>> A(
      {{1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}});
  BitVectorSet<std::pair<int, int>> B({{1, 1}, {2, 2}, {3, 3}});
  BitVectorSet<std::pair<int, int>> C({{1, 1}, {2, 2}, {42, 42}});
  BitVectorSet<std::pair<int, int>> D(
      {{1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}, {7, 7}});
  BitVectorSet<std::pair<int, int>> E({{1, 1}, {2, 2}, {3, 3}});
  BitVectorSet<std::pair<int, int>> F({{1, 2}, {2, 2}, {3, 3}});

  EXPECT_TRUE(A.includes(B));
  EXPECT_FALSE(B.includes(A));
  EXPECT_FALSE(A.includes(C));
  EXPECT_FALSE(B.includes(C));
  EXPECT_FALSE(A.includes(D));
  EXPECT_TRUE(D.includes(A));
  EXPECT_TRUE(B.includes(E));
  EXPECT_TRUE(E.includes(B));
  EXPECT_FALSE(F.includes(B));
}

TEST(BitVectorSet, includesForIntegersTakeTwo) {
  BitVectorSet<int> X({1, 2, 3, 4, 5, 6, 7});
  BitVectorSet<int> Y({7});

  EXPECT_TRUE(X.includes(Y));
}

TEST(BitVectorSet, iterator) {
  BitVectorSet<int> A({10, 20, 30, 40, 50});
  BitVectorSet<int> D({10, 20, 30, 40, 50});

  auto IteratorD = D.begin();
  for (auto It = A.begin(); It != A.end(); It++) {
    EXPECT_EQ(*It, *IteratorD);
    IteratorD++;
  }

  BitVectorSet<int> E({25, 32, 40, 57});
  std::set<int> ES;
  std::set<int> ESGT = {25, 32, 40, 57};
  for (auto It = E.begin(); It != E.end(); It++) {
    // EXPECT_TRUE(ESGT.find(*it)!=ESGT.end()); //Extra check
    ES.insert(*It);
  }
  EXPECT_EQ(ES, ESGT);
}

TEST(BitVectorSet, iterator_movement) {
  BitVectorSet<int> A({10, 20, 30, 40, 50});
  BitVectorSet<int> B({30, 40, 50, 60});
  auto IteratorA = A.begin();
  auto IteratorB = B.begin();

  IteratorA += 4;
  IteratorB += 2;
  EXPECT_EQ(*IteratorA, *IteratorB);
  EXPECT_EQ(A.count(*IteratorA), 1U);
  IteratorB++;
  EXPECT_EQ(A.count(*IteratorB), 0U);
}

TEST(BitVectorSet, rangeFor) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  std::set<int> AS;
  std::set<int> ASGT = {1, 2, 3, 4, 5, 6};

  for (auto I : A) {
    AS.insert(I);
  }
  EXPECT_EQ(AS, ASGT);

  BitVectorSet<int> B({6, 5, 4, 3, 2, 1});
  std::set<int> BS;
  std::set<int> BSGT = {6, 5, 4, 3, 2, 1};
  for (auto I : B) {
    BS.insert(I);
  }
  EXPECT_EQ(BS, BSGT);

  BitVectorSet<int> C({5, 6, 7, 8, 42, 13});
  std::set<int> CS;
  std::set<int> CSGT = {5, 6, 7, 8, 42, 13};
  for (auto I : C) {
    CS.insert(I);
  }
  EXPECT_EQ(CS, CSGT);

  const BitVectorSet<int> D({5, 6, 7, 8, 42, 13});
  std::set<int> DS;
  std::set<int> DSGT = {5, 6, 7, 8, 42, 13};
  auto I = D.begin();
  for (auto I : D) {
    DS.insert(I);
  }
  EXPECT_EQ(DS, DSGT);
}

TEST(BitVectorSet, lessThan) {
  BitVectorSet<int> A({1, 2, 3, 4, 5, 6});
  BitVectorSet<int> B({1, 2, 3, 4, 5});

  EXPECT_TRUE(B < A);
  EXPECT_FALSE(A < B);
  EXPECT_FALSE(A < A);
}

TEST(BitVectorSet, hash) {
  struct Hasher {
    size_t operator()(const BitVectorSet<int> &BV) const {
      return hash_value(BV);
    }
  };

  std::unordered_set<BitVectorSet<int>, Hasher> Set;
  Set.insert(BitVectorSet<int>());
  Set.insert({1, 3});
  Set.insert({1, 3});
  Set.insert({1, 3, 5});

  EXPECT_EQ(3, Set.size());
  EXPECT_TRUE(Set.count({})) << "Empty set not there in " << PrettyPrinter{Set};
  EXPECT_TRUE(Set.count({1, 3}))
      << "{1, 3} not there in " << PrettyPrinter{Set};
  EXPECT_TRUE(Set.count({1, 3, 5}))
      << "{1, 3, 5} not there in " << PrettyPrinter{Set};
}

//===----------------------------------------------------------------------===//
// llvm::BitVector

TEST(BitVector, emptyVectorsShouldNotBeLess) {
  llvm::BitVector A(5);
  llvm::BitVector B(5);

  EXPECT_FALSE(internal::isLess(A, B));
  EXPECT_FALSE(internal::isLess(B, A));
}

TEST(BitVector, emptyVectorsWithDifferentSizeShouldNotBeLess) {
  llvm::BitVector A(42);
  llvm::BitVector B(5);

  EXPECT_FALSE(internal::isLess(A, B));
  EXPECT_FALSE(internal::isLess(B, A));
}

TEST(BitVector, equalVectorsShouldNotBeLess) {
  llvm::BitVector A(10);
  llvm::BitVector B(10);

  A.set(3);
  B.set(3);
  A.set(5);
  B.set(5);
  A.set(8);
  B.set(8);

  EXPECT_FALSE(internal::isLess(A, B));
  EXPECT_FALSE(internal::isLess(B, A));
}

TEST(BitVector, equalSizedVectorsWithBitDifference) {
  llvm::BitVector A(10);
  llvm::BitVector B(10);

  A.set(3);
  B.set(3);
  A.set(5);
  B.set(4); // B has a lower bit set than A
  A.set(8);
  B.set(8);

  EXPECT_FALSE(internal::isLess(A, B));
  EXPECT_TRUE(internal::isLess(B, A));

  // Switching the bits shoud invert the lesser relationship
  A.set(4);
  A.reset(5);
  B.set(5);
  B.reset(4);

  EXPECT_TRUE(internal::isLess(A, B));
  EXPECT_FALSE(internal::isLess(B, A));
}

TEST(BitVector, biggerVecWithoutUpperBitsSetShouldNotBeLess) {
  llvm::BitVector A(42);
  llvm::BitVector B(10);

  A.set(5);
  B.set(4); // B has a lower bit set than A

  EXPECT_FALSE(internal::isLess(A, B));
  EXPECT_TRUE(internal::isLess(B, A));
}

TEST(BitVector, biggerVecWithUpperBitsSetShouldBeLess) {
  llvm::BitVector A(42);
  llvm::BitVector B(10);

  A.set(5);
  B.set(4); // B has a lower bit set than A

  A.set(40);

  EXPECT_FALSE(internal::isLess(A, B));
  EXPECT_TRUE(internal::isLess(B, A));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
