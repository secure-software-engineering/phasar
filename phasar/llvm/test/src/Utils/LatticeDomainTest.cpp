#include "gtest/gtest.h"

#include "phasar/PhasarLLVM/Utils/LatticeDomain.h"

using namespace psr;

TEST(LatticeDomain, topShouldNotBeLessThanTop) {
  LatticeDomain<int> LD1(Top{});
  LatticeDomain<int> LD2(Top{});

  EXPECT_FALSE(LD1 < LD2);
  EXPECT_FALSE(LD2 < LD1);
}

TEST(LatticeDomain, bottomShouldNotBeLessThanBottom) {
  LatticeDomain<int> LD1(Bottom{});
  LatticeDomain<int> LD2(Bottom{});

  EXPECT_FALSE(LD1 < LD2);
  EXPECT_FALSE(LD2 < LD1);
}

TEST(LatticeDomain, topShouldBeLessThanBottom) {
  LatticeDomain<int> LD1(Top{});
  LatticeDomain<int> LD2(Bottom{});

  EXPECT_TRUE(LD1 < LD2);
}

TEST(LatticeDomain, bottomShouldNotBeLessThanTop) {
  LatticeDomain<int> LD1(Top{});
  LatticeDomain<int> LD2(Bottom{});

  EXPECT_FALSE(LD2 < LD1);
}

TEST(LatticeDomain, lessShouldCorrectlyForwardToInnerLatticeType) {
  LatticeDomain<int> LD1(42);
  LatticeDomain<int> LD2(21);

  EXPECT_FALSE(LD1 < LD2);
  EXPECT_TRUE(LD2 < LD1);
}

TEST(LatticeDomain, innerLatticeTypeShouldBeNotLessThanTop) {
  LatticeDomain<int> LD1(42);
  LatticeDomain<int> LD2(Top{});

  EXPECT_FALSE(LD1 < LD2);
  EXPECT_TRUE(LD2 < LD1);
}

TEST(LatticeDomain, innerLatticeTypeShouldBeLessThanBottom) {
  LatticeDomain<int> LD1(42);
  LatticeDomain<int> LD2(Bottom{});

  EXPECT_TRUE(LD1 < LD2);
  EXPECT_FALSE(LD2 < LD1);
}
