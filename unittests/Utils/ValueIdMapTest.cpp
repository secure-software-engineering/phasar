/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/ValueIdMap.h"

#include "gtest/gtest.h"

#include <stdexcept>
#include <string>
#include <utility>

using namespace psr;

enum class Id : uint32_t {};

// ── Helpers ──────────────────────────────────────────────────────────────────

static constexpr Id id(uint32_t V) noexcept { return Id(V); }

template <typename MapT> static void expectEmpty(const MapT &M) {
  EXPECT_TRUE(M.empty());
  EXPECT_EQ(0u, M.size());
  EXPECT_EQ(M.end(), M.begin());
}

// ── Basic construction
// ────────────────────────────────────────────────────────

TEST(ValueIdMap, DefaultCtorEmpty) {
  ValueIdMap<Id, int> M;
  expectEmpty(M);
  EXPECT_FALSE(M.contains(id(0)));
  EXPECT_EQ(0u, M.count(id(0)));
}

TEST(ValueIdMap, ReserveCtorEmpty) {
  ValueIdMap<Id, int> M(64);
  expectEmpty(M);
  EXPECT_FALSE(M.contains(id(0)));
}

// ── try_emplace
// ───────────────────────────────────────────────────────────────

TEST(ValueIdMap, TryEmplaceInsert) {
  ValueIdMap<Id, int> M;
  auto [It, Inserted] = M.try_emplace(id(3), 42);
  EXPECT_TRUE(Inserted);
  EXPECT_EQ(id(3), It->first);
  EXPECT_EQ(42, It->second);
  EXPECT_EQ(1u, M.size());
  EXPECT_TRUE(M.contains(id(3)));
}

TEST(ValueIdMap, TryEmplaceDuplicate) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(3), 42);
  auto [It, Inserted] = M.try_emplace(id(3), 99);
  EXPECT_FALSE(Inserted);
  EXPECT_EQ(42, It->second); // original value unchanged
  EXPECT_EQ(1u, M.size());
}

TEST(ValueIdMap, TryEmplaceMultiple) {
  ValueIdMap<Id, int> M;
  for (uint32_t I = 0; I < 10; ++I) {
    auto [It, Inserted] = M.try_emplace(id(I), int(I * 2));
    EXPECT_TRUE(Inserted);
    EXPECT_EQ(int(I * 2), It->second);
  }
  EXPECT_EQ(10u, M.size());
  for (uint32_t I = 0; I < 10; ++I) {
    EXPECT_TRUE(M.contains(id(I)));
    EXPECT_EQ(int(I * 2), M.at(id(I)));
  }
}

// ── operator[] ───────────────────────────────────────────────────────────────

TEST(ValueIdMap, OperatorBracketInserts) {
  ValueIdMap<Id, int> M;
  M[id(5)] = 77;
  EXPECT_EQ(1u, M.size());
  EXPECT_EQ(77, M.at(id(5)));
}

TEST(ValueIdMap, OperatorBracketExisting) {
  ValueIdMap<Id, int> M;
  M[id(5)] = 77;
  M[id(5)] = 99;
  EXPECT_EQ(1u, M.size());
  EXPECT_EQ(99, M.at(id(5)));
}

TEST(ValueIdMap, OperatorBracketDefaultConstructs) {
  ValueIdMap<Id, int> M;
  int &Ref = M[id(2)];
  EXPECT_EQ(0,
            Ref); // int default-initialised to 0 via value-init in try_emplace
  EXPECT_EQ(1u, M.size());
}

// ── insert / insert_or_assign
// ─────────────────────────────────────────────────

TEST(ValueIdMap, InsertNew) {
  ValueIdMap<Id, int> M;
  auto [It, Inserted] = M.insert({id(7), 100});
  EXPECT_TRUE(Inserted);
  EXPECT_EQ(100, It->second);
}

TEST(ValueIdMap, InsertExisting) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(7), 100);
  auto [It, Inserted] = M.insert({id(7), 200});
  EXPECT_FALSE(Inserted);
  EXPECT_EQ(100, It->second);
}

TEST(ValueIdMap, InsertOrAssignNew) {
  ValueIdMap<Id, int> M;
  auto [It, Inserted] = M.insert_or_assign(id(4), 55);
  EXPECT_TRUE(Inserted);
  EXPECT_EQ(55, It->second);
}

TEST(ValueIdMap, InsertOrAssignExisting) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(4), 55);
  auto [It, Inserted] = M.insert_or_assign(id(4), 88);
  EXPECT_FALSE(Inserted);
  EXPECT_EQ(88, It->second);
  EXPECT_EQ(88, M.at(id(4)));
}

// ── contains / count / find
// ───────────────────────────────────────────────────

TEST(ValueIdMap, ContainsFindAbsent) {
  ValueIdMap<Id, int> M;
  EXPECT_FALSE(M.contains(id(0)));
  EXPECT_EQ(0u, M.count(id(0)));
  EXPECT_EQ(M.end(), M.find(id(0)));
}

TEST(ValueIdMap, ContainsFindPresent) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(0), 1);
  EXPECT_TRUE(M.contains(id(0)));
  EXPECT_EQ(1u, M.count(id(0)));
  auto It = M.find(id(0));
  ASSERT_NE(M.end(), It);
  EXPECT_EQ(1, It->second);
}

// ── at
// ────────────────────────────────────────────────────────────────────────

TEST(ValueIdMap, AtPresent) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(2), 42);
  EXPECT_EQ(42, M.at(id(2)));
  M.at(id(2)) = 99;
  EXPECT_EQ(99, M.at(id(2)));
}

TEST(ValueIdMap, AtAbsentThrows) {
  ValueIdMap<Id, int> M;
  EXPECT_THROW(std::ignore = M.at(id(0)), std::out_of_range);
}

TEST(ValueIdMap, AtConstPresent) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(2), 42);
  const auto &CM = M;
  EXPECT_EQ(42, CM.at(id(2)));
}

TEST(ValueIdMap, AtConstAbsentThrows) {
  const ValueIdMap<Id, int> M;
  EXPECT_THROW(std::ignore = M.at(id(0)), std::out_of_range);
}

// ── erase
// ─────────────────────────────────────────────────────────────────────

TEST(ValueIdMap, EraseKeyPresent) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(1), 10);
  EXPECT_TRUE(M.erase(id(1)));
  EXPECT_EQ(0u, M.size());
  EXPECT_FALSE(M.contains(id(1)));
}

TEST(ValueIdMap, EraseKeyAbsent) {
  ValueIdMap<Id, int> M;
  EXPECT_FALSE(M.erase(id(1)));
}

TEST(ValueIdMap, EraseIterator) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(1), 10);
  M.try_emplace(id(3), 30);
  M.try_emplace(id(5), 50);

  auto It = M.find(id(3));
  ASSERT_NE(M.end(), It);
  It = M.erase(It);

  EXPECT_EQ(2u, M.size());
  EXPECT_FALSE(M.contains(id(3)));
  // Iterator should now point at id(5) (the next key)
  ASSERT_NE(M.end(), It);
  EXPECT_EQ(id(5), It->first);
}

TEST(ValueIdMap, EraseIteratorLast) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(2), 20);
  auto It = M.erase(M.begin());
  EXPECT_EQ(M.end(), It);
  EXPECT_TRUE(M.empty());
}

// ── clear
// ─────────────────────────────────────────────────────────────────────

TEST(ValueIdMap, ClearInt) {
  ValueIdMap<Id, int> M;
  for (uint32_t I = 0; I < 8; ++I) {
    M.try_emplace(id(I), int(I));
  }
  M.clear();
  expectEmpty(M);
  EXPECT_FALSE(M.contains(id(0)));
}

TEST(ValueIdMap, ClearString) {
  ValueIdMap<Id, std::string> M;
  for (uint32_t I = 0; I < 8; ++I) {
    M.try_emplace(id(I), std::to_string(I));
  }
  M.clear();
  expectEmpty(M);
}

TEST(ValueIdMap, ReuseAfterClear) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(0), 1);
  M.clear();
  M.try_emplace(id(0), 2);
  EXPECT_EQ(1u, M.size());
  EXPECT_EQ(2, M.at(id(0)));
}

// ── Iteration
// ─────────────────────────────────────────────────────────────────

TEST(ValueIdMap, IterationOrder) {
  // Keys must be visited in ascending order (BitSet property)
  ValueIdMap<Id, int> M;
  M.try_emplace(id(5), 50);
  M.try_emplace(id(1), 10);
  M.try_emplace(id(3), 30);

  std::vector<uint32_t> Keys;
  for (auto [K, V] : M) {
    Keys.push_back(uint32_t(K));
  }
  ASSERT_EQ(3u, Keys.size());
  EXPECT_EQ(1u, Keys[0]);
  EXPECT_EQ(3u, Keys[1]);
  EXPECT_EQ(5u, Keys[2]);
}

TEST(ValueIdMap, ConstIteration) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(2), 20);
  M.try_emplace(id(4), 40);

  const auto &CM = M;
  int Sum = 0;
  for (auto [K, V] : CM) {
    Sum += V;
  }
  EXPECT_EQ(60, Sum);
}

TEST(ValueIdMap, IteratorToConstIterator) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(0), 7);
  ValueIdMap<Id, int>::iterator It = M.begin();
  ValueIdMap<Id, int>::const_iterator CIt = It; // implicit conversion
  EXPECT_EQ(It->second, CIt->second);
}

// ── Copy / Move
// ───────────────────────────────────────────────────────────────

TEST(ValueIdMap, CopyCtorInt) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(1), 10);
  M.try_emplace(id(3), 30);

  ValueIdMap<Id, int> Copy(M);
  EXPECT_EQ(2u, Copy.size());
  EXPECT_EQ(10, Copy.at(id(1)));
  EXPECT_EQ(30, Copy.at(id(3)));

  // Independent
  M.at(id(1)) = 99;
  EXPECT_EQ(10, Copy.at(id(1)));
}

TEST(ValueIdMap, CopyCtorString) {
  ValueIdMap<Id, std::string> M;
  M.try_emplace(id(0), "hello");
  M.try_emplace(id(2), "world");

  ValueIdMap<Id, std::string> Copy(M);
  EXPECT_EQ("hello", Copy.at(id(0)));
  EXPECT_EQ("world", Copy.at(id(2)));
}

TEST(ValueIdMap, CopyCtorEmpty) {
  ValueIdMap<Id, int> M;
  ValueIdMap<Id, int> Copy(M);
  expectEmpty(Copy);
}

TEST(ValueIdMap, MoveCtorInt) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(1), 10);
  M.try_emplace(id(3), 30);

  ValueIdMap<Id, int> Moved(std::move(M));
  EXPECT_EQ(2u, Moved.size());
  EXPECT_EQ(10, Moved.at(id(1)));
  EXPECT_EQ(30, Moved.at(id(3)));
}

TEST(ValueIdMap, CopyAssign) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(0), 5);
  ValueIdMap<Id, int> Other;
  Other = M;
  EXPECT_EQ(5, Other.at(id(0)));
}

TEST(ValueIdMap, MoveAssign) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(0), 5);
  ValueIdMap<Id, int> Other;
  Other = std::move(M);
  EXPECT_EQ(5, Other.at(id(0)));
}

TEST(ValueIdMap, SelfAssign) {
  ValueIdMap<Id, int> M;
  M.try_emplace(id(0), 42);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
  M = M; // NOLINT(clang-diagnostic-self-assign-overloaded)
#pragma GCC diagnostic pop
  EXPECT_EQ(42, M.at(id(0)));
}

// ── Reallocation (grow)
// ───────────────────────────────────────────────────────

TEST(ValueIdMap, GrowTrivial) {
  // Insert enough keys to force multiple reallocations
  ValueIdMap<Id, int> M(4);
  for (uint32_t I = 0; I < 100; ++I) {
    M.try_emplace(id(I), int(I));
  }
  EXPECT_EQ(100u, M.size());
  for (uint32_t I = 0; I < 100; ++I) {
    EXPECT_EQ(int(I), M.at(id(I)));
  }
}

TEST(ValueIdMap, GrowNonTrivial) {
  // std::string is not trivially relocatable; exercises the move path in grow()
  ValueIdMap<Id, std::string> M(4);
  for (uint32_t I = 0; I < 100; ++I) {
    M.try_emplace(id(I), std::to_string(I));
  }
  EXPECT_EQ(100u, M.size());
  for (uint32_t I = 0; I < 100; ++I) {
    EXPECT_EQ(std::to_string(I), M.at(id(I)));
  }
}

TEST(ValueIdMap, GrowSparseKeys) {
  // Sparse insertion: only a few high-valued keys, triggering capacity growth
  ValueIdMap<Id, int> M;
  M.try_emplace(id(0), 0);
  M.try_emplace(id(63), 63);
  M.try_emplace(id(127), 127);
  EXPECT_EQ(3u, M.size());
  EXPECT_EQ(0, M.at(id(0)));
  EXPECT_EQ(63, M.at(id(63)));
  EXPECT_EQ(127, M.at(id(127)));
}

// ── Swap
// ──────────────────────────────────────────────────────────────────────

TEST(ValueIdMap, Swap) {
  ValueIdMap<Id, int> A;
  A.try_emplace(id(1), 10);
  ValueIdMap<Id, int> B;
  B.try_emplace(id(2), 20);
  B.try_emplace(id(3), 30);

  swap(A, B);
  EXPECT_EQ(2u, A.size());
  EXPECT_EQ(20, A.at(id(2)));
  EXPECT_EQ(30, A.at(id(3)));
  EXPECT_EQ(1u, B.size());
  EXPECT_EQ(10, B.at(id(1)));
}

// ── reserve
// ───────────────────────────────────────────────────────────────────

TEST(ValueIdMap, ReserveNoConstruct) {
  ValueIdMap<Id, int> M;
  M.reserve(64);
  expectEmpty(M);
  EXPECT_FALSE(M.contains(id(0)));
  // Inserting within reserved capacity should not reallocate
  M.try_emplace(id(10), 100);
  EXPECT_EQ(100, M.at(id(10)));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
