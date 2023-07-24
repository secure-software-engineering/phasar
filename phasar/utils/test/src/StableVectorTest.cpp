#include "phasar/Utils/StableVector.h"

#include "gtest/gtest.h"

#include <iterator>
#include <string>

#include <gtest/gtest-param-test.h>

using namespace psr;

static StableVector<int> makeVecInt(int Size) {
  StableVector<int> Vec;
  for (int I = 0; I < Size; ++I) {
    Vec.push_back(I);
  }
  return Vec;
}

static StableVector<std::string> makeVecString(int Size) {
  StableVector<std::string> Vec;
  for (int I = 0; I < Size; ++I) {
    Vec.push_back(std::to_string(I));
  }
  return Vec;
}

template <typename VecTy> static void expectEmpty(const VecTy &Vec) {
  EXPECT_TRUE(Vec.empty());
  EXPECT_EQ(0, Vec.size());
  EXPECT_EQ(Vec.end(), Vec.begin());
  EXPECT_EQ(Vec.cend(), Vec.cbegin());
}

template <typename VecTy> static void expectNonEmpty(const VecTy &Vec) {
  EXPECT_FALSE(Vec.empty());
  EXPECT_NE(0, Vec.size());
  EXPECT_NE(Vec.end(), Vec.begin());
  EXPECT_NE(Vec.cend(), Vec.cbegin());
}

class Insert : public ::testing::TestWithParam<int> {};
class Iter : public ::testing::TestWithParam<int> {};
class Idx : public ::testing::TestWithParam<int> {};
class Pop : public ::testing::TestWithParam<int> {};
class PopVal : public ::testing::TestWithParam<int> {};
class PopN : public ::testing::TestWithParam<int> {};
class Copy : public ::testing::TestWithParam<int> {};
class Clear : public ::testing::TestWithParam<int> {};

TEST(Basic, DefaultCtor) {
  StableVector<int> Vec;
  expectEmpty(Vec);
}

TEST(Basic, OneElement) {
  StableVector<int> Vec;
  Vec.push_back(42);
  expectNonEmpty(Vec);
  EXPECT_EQ(1, Vec.size());
  EXPECT_EQ(42, *Vec.begin());
  EXPECT_EQ(42, *Vec.cbegin());
  EXPECT_EQ(Vec.end(), std::next(Vec.begin()));
  EXPECT_EQ(Vec.cend(), std::next(Vec.cbegin()));
}

TEST_P(Insert, Int) {
  auto Size = GetParam();

  StableVector<int> Vec;
  for (int I = 0; I < Size; ++I) {
    EXPECT_EQ(size_t(I), Vec.size());
    Vec.push_back(I);
    EXPECT_FALSE(Vec.empty());
    EXPECT_EQ(size_t(I) + 1, Vec.size());
    EXPECT_EQ(I, Vec.back());
  }

  EXPECT_EQ(Size, Vec.size());
}

TEST_P(Insert, String) {
  auto Size = GetParam();

  StableVector<std::string> Vec;
  for (int I = 0; I < Size; ++I) {
    EXPECT_EQ(size_t(I), Vec.size());
    Vec.push_back(std::to_string(I));
    EXPECT_FALSE(Vec.empty());
    EXPECT_EQ(size_t(I) + 1, Vec.size());
    EXPECT_EQ(std::to_string(I), Vec.back());
  }

  EXPECT_EQ(Size, Vec.size());
}

TEST_P(Iter, Int) {
  auto Size = GetParam();
  auto Vec = makeVecInt(Size);

  int I = 0;
  for (int Elem : Vec) {
    EXPECT_EQ(I, Elem);
    ++I;
  }
  EXPECT_EQ(Size, I);
}

TEST_P(Iter, String) {
  auto Size = GetParam();
  auto Vec = makeVecString(Size);

  int I = 0;
  for (std::string &Elem : Vec) {
    EXPECT_EQ(std::to_string(I), Elem);
    ++I;
  }
  EXPECT_EQ(Size, I);
}

TEST_P(Idx, Int) {
  auto Size = GetParam();
  auto Vec = makeVecInt(Size);

  for (int I = 0; I < Size; ++I) {
    EXPECT_EQ(I, Vec[I]);
  }
}

TEST_P(Idx, String) {
  auto Size = GetParam();
  auto Vec = makeVecString(Size);

  for (int I = 0; I < Size; ++I) {
    EXPECT_EQ(std::to_string(I), Vec[I]);
  }
}

TEST_P(Pop, Int) {
  auto Size = GetParam();
  auto Vec = makeVecInt(Size);
  for (int I = 0; I < Size; ++I) {
    ASSERT_FALSE(Vec.empty());
    EXPECT_EQ(Size - 1 - I, Vec.back());
    Vec.pop_back();
  }
  expectEmpty(Vec);
}

TEST_P(Pop, String) {
  auto Size = GetParam();
  auto Vec = makeVecString(Size);
  for (int I = 0; I < Size; ++I) {
    ASSERT_FALSE(Vec.empty());
    EXPECT_EQ(std::to_string(Size - 1 - I), Vec.back());
    Vec.pop_back();
  }
  expectEmpty(Vec);
}

TEST_P(PopVal, Int) {
  auto Size = GetParam();
  auto Vec = makeVecInt(Size);
  for (int I = 0; I < Size; ++I) {
    ASSERT_FALSE(Vec.empty());
    EXPECT_EQ(Size - 1 - I, Vec.back());
    auto Val = Vec.pop_back_val();
    EXPECT_EQ(Size - 1 - I, Val);
  }
  expectEmpty(Vec);
}

TEST_P(PopVal, String) {
  auto Size = GetParam();
  auto Vec = makeVecString(Size);
  for (int I = 0; I < Size; ++I) {
    ASSERT_FALSE(Vec.empty());
    EXPECT_EQ(std::to_string(Size - 1 - I), Vec.back());
    auto Val = Vec.pop_back_val();
    EXPECT_EQ(std::to_string(Size - 1 - I), Val);
  }
  expectEmpty(Vec);
}

TEST_P(PopN, Int) {
  auto Size = GetParam();
  auto Vec = makeVecInt(Size);
  expectNonEmpty(Vec);

  Vec.pop_back_n(Size / 2);
  expectNonEmpty(Vec);
  EXPECT_EQ(Size / 2, Vec.size());
  EXPECT_EQ(Size / 2 - 1, Vec.back());

  Vec.pop_back_n(Size / 2);
  expectEmpty(Vec);
}

TEST_P(PopN, String) {
  auto Size = GetParam();
  auto Vec = makeVecString(Size);
  expectNonEmpty(Vec);

  Vec.pop_back_n(Size / 2);
  expectNonEmpty(Vec);
  EXPECT_EQ(Size / 2, Vec.size());
  EXPECT_EQ(std::to_string(Size / 2 - 1), Vec.back());

  Vec.pop_back_n(Size / 2);
  expectEmpty(Vec);
}

TEST_P(Copy, Int) {
  auto Size = GetParam();
  auto Vec = makeVecInt(Size);
  expectNonEmpty(Vec);
  EXPECT_EQ(Size, Vec.size());
  auto Vec2 = Vec.clone();
  expectNonEmpty(Vec);
  expectNonEmpty(Vec2);
  EXPECT_EQ(Vec, Vec2);
}

TEST_P(Copy, IntPush) {
  auto Size = GetParam();
  auto Vec = makeVecInt(Size);
  expectNonEmpty(Vec);
  EXPECT_EQ(Size, Vec.size());
  auto Vec2 = Vec.clone();
  Vec.push_back(3);
  expectNonEmpty(Vec);
  expectNonEmpty(Vec2);
  EXPECT_NE(Vec, Vec2);
}

TEST_P(Copy, String) {
  auto Size = GetParam();
  auto Vec = makeVecString(Size);
  expectNonEmpty(Vec);
  EXPECT_EQ(Size, Vec.size());
  auto Vec2 = Vec.clone();
  expectNonEmpty(Vec);
  expectNonEmpty(Vec2);
  EXPECT_EQ(Vec, Vec2);
}

TEST_P(Copy, StringPush) {
  auto Size = GetParam();
  auto Vec = makeVecString(Size);
  expectNonEmpty(Vec);
  EXPECT_EQ(Size, Vec.size());
  auto Vec2 = Vec.clone();
  Vec.push_back("Hello, World!");
  expectNonEmpty(Vec);
  expectNonEmpty(Vec2);
  EXPECT_NE(Vec, Vec2);
}

TEST_P(Clear, Int) {
  auto Size = GetParam();
  auto Vec = makeVecInt(Size);
  expectNonEmpty(Vec);
  Vec.clear();
  expectEmpty(Vec);
}

TEST_P(Clear, String) {
  auto Size = GetParam();
  auto Vec = makeVecString(Size);
  expectNonEmpty(Vec);
  Vec.clear();
  expectEmpty(Vec);
}

INSTANTIATE_TEST_SUITE_P(StableVector, Insert,
                         ::testing::Values<int>(100, 200, 300, 511, 512));
INSTANTIATE_TEST_SUITE_P(StableVector, Iter,
                         ::testing::Values<int>(100, 200, 300, 511, 512));
INSTANTIATE_TEST_SUITE_P(StableVector, Idx,
                         ::testing::Values<int>(100, 200, 300, 511, 512));
INSTANTIATE_TEST_SUITE_P(StableVector, Pop,
                         ::testing::Values<int>(100, 200, 300, 511, 512));
INSTANTIATE_TEST_SUITE_P(StableVector, PopVal,
                         ::testing::Values<int>(100, 200, 300, 512));
INSTANTIATE_TEST_SUITE_P(StableVector, PopN,
                         ::testing::Values<int>(100, 200, 300, 512));
INSTANTIATE_TEST_SUITE_P(StableVector, Copy,
                         ::testing::Values<int>(100, 200, 300, 511, 512));
INSTANTIATE_TEST_SUITE_P(StableVector, Clear,
                         ::testing::Values<int>(100, 200, 300, 511, 512));
