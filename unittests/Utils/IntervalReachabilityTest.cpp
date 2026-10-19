/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/IntervalReachability.h"

#include "phasar/Utils/BitSet.h"
#include "phasar/Utils/CsrGraph.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/IotaIterator.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include "gtest/gtest.h"

#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <utility>

namespace {

enum class VtxId : uint32_t {};

using Graph = psr::CsrGraph<VtxId>;
using Edge = std::pair<VtxId, VtxId>;
using Index = psr::IntervalReachabilityIndex<VtxId>;
using Builder = psr::IntervalReachabilityBuilder<VtxId>;
using VtxSet = psr::BitSet<VtxId>;

static_assert(psr::is_const_graph<Graph>);

constexpr VtxId operator""_v(unsigned long long Id) { return VtxId(Id); }

Graph makeGraph(size_t NumVertices, llvm::ArrayRef<Edge> Edges) {
  return Graph::fromEdges(NumVertices, Edges);
}

/// Vertices reachable from Root, including Root itself
VtxSet reachableFrom(const Graph &G, VtxId Root) {
  VtxSet Seen(G.numVertices());
  llvm::SmallVector<VtxId> WL{Root};
  Seen.insert(Root);
  while (!WL.empty()) {
    auto Vtx = WL.pop_back_val();
    for (auto Succ : G.succsOf(Vtx)) {
      if (Seen.tryInsert(Succ)) {
        WL.push_back(Succ);
      }
    }
  }
  return Seen;
}

/// Checks the index against a brute-force transitive closure
void checkIndex(const Graph &G, llvm::ArrayRef<VtxId> Roots,
                const VtxSet &IsTarget, const Index &Idx) {
  ASSERT_EQ(Idx.numVertices(), G.numVertices());

  VtxSet Indexed(G.numVertices());
  for (auto Root : Roots) {
    Indexed |= reachableFrom(G, Root);
  }

  size_t NumTargets = 0;
  for (auto Vtx : psr::iota<VtxId>(G.numVertices())) {
    EXPECT_EQ(Idx.isIndexed(Vtx), Indexed.contains(Vtx))
        << "Vertex " << uint32_t(Vtx);
    bool ExpectTarget = Indexed.contains(Vtx) && IsTarget.contains(Vtx);
    EXPECT_EQ(Idx.isTarget(Vtx), ExpectTarget) << "Vertex " << uint32_t(Vtx);
    if (ExpectTarget) {
      ++NumTargets;
      EXPECT_EQ(Idx.vertexOfRank(Idx.rankOf(Vtx)), Vtx);
    }
  }
  EXPECT_EQ(Idx.numTargets(), NumTargets);

  for (auto From : Indexed) {
    auto Reachable = reachableFrom(G, From);

    VtxSet ExpectedTargets(G.numVertices());
    for (auto To : psr::iota<VtxId>(G.numVertices())) {
      bool Expected = Reachable.contains(To) && IsTarget.contains(To);
      if (Expected) {
        ExpectedTargets.insert(To);
      }
      EXPECT_EQ(Idx.reaches(From, To), Expected)
          << "From " << uint32_t(From) << " To " << uint32_t(To);
    }

    std::optional<psr::ReachRank> PrevHi;
    Idx.forEachReachableRange(From, [&](psr::RankInterval Interval) {
      EXPECT_FALSE(Interval.empty());
      if (PrevHi) {
        EXPECT_LE(uint32_t(*PrevHi), uint32_t(Interval.Lo))
            << "Intervals not sorted or not disjoint";
      }
      PrevHi = Interval.Hi;
    });

    VtxSet Enumerated(G.numVertices());
    Idx.forEachReachableTarget(From, [&](VtxId To) {
      EXPECT_TRUE(Enumerated.tryInsert(To)) << "Duplicate target";
    });
    EXPECT_TRUE(Enumerated == ExpectedTargets) << "From " << uint32_t(From);

    // SCC ids are in reverse topological order
    for (auto Succ : G.succsOf(From)) {
      EXPECT_LE(+Idx.sccOf(Succ), +Idx.sccOf(From));
    }
  }
}

void checkIndex(const Graph &G, llvm::ArrayRef<VtxId> Roots, const Index &Idx) {
  checkIndex(G, Roots, VtxSet(G.numVertices(), true), Idx);
}

TEST(CsrGraphTest, FromEdgesKeepsEdgeOrder) {
  auto G = makeGraph(4, {{2_v, 1_v}, {0_v, 3_v}, {2_v, 0_v}, {0_v, 1_v}});

  ASSERT_EQ(G.numVertices(), 4U);
  ASSERT_EQ(G.numEdges(), 4U);
  EXPECT_EQ(G.succsOf(0_v), llvm::ArrayRef<VtxId>({3_v, 1_v}));
  EXPECT_TRUE(G.succsOf(1_v).empty());
  EXPECT_EQ(G.succsOf(2_v), llvm::ArrayRef<VtxId>({1_v, 0_v}));
  EXPECT_TRUE(G.succsOf(3_v).empty());
}

TEST(CsrGraphTest, Reversed) {
  auto G = makeGraph(4, {{2_v, 1_v}, {0_v, 3_v}, {2_v, 0_v}, {0_v, 1_v}});
  auto Rev = Graph::reversed(G);

  ASSERT_EQ(Rev.numVertices(), 4U);
  ASSERT_EQ(Rev.numEdges(), 4U);
  EXPECT_EQ(Rev.succsOf(0_v), llvm::ArrayRef<VtxId>({2_v}));
  EXPECT_EQ(Rev.succsOf(1_v), llvm::ArrayRef<VtxId>({0_v, 2_v}));
  EXPECT_TRUE(Rev.succsOf(2_v).empty());
  EXPECT_EQ(Rev.succsOf(3_v), llvm::ArrayRef<VtxId>({0_v}));
  EXPECT_EQ(Rev.roots(), (llvm::ArrayRef<VtxId>{1_v, 3_v}));
}

TEST(CsrGraphTest, AssignEdgesReusesGraph) {
  auto G = makeGraph(3, {{0_v, 1_v}, {1_v, 2_v}});
  G.assignEdges(2, {{1_v, 0_v}});

  ASSERT_EQ(G.numVertices(), 2U);
  ASSERT_EQ(G.numEdges(), 1U);
  EXPECT_TRUE(G.succsOf(0_v).empty());
  EXPECT_EQ(G.succsOf(1_v), llvm::ArrayRef<VtxId>({0_v}));
}

TEST(IntervalReachabilityTest, Chain) {
  auto G = makeGraph(4, {{0_v, 1_v}, {1_v, 2_v}, {2_v, 3_v}});
  Index Idx = Builder().build(G, {0_v});

  checkIndex(G, {0_v}, Idx);
  EXPECT_EQ(Idx.numSCCs(), 4U);
  for (auto Vtx : psr::iota<VtxId>(4)) {
    EXPECT_EQ(Idx.numIntervalsOf(Idx.sccOf(Vtx)), 1U);
  }
}

TEST(IntervalReachabilityTest, SharedSuccessorIsNoFalsePositive) {
  // 1 and 2 both reach 3 via a tree- resp. cross-edge, but not each other
  auto G = makeGraph(4, {{0_v, 1_v}, {0_v, 2_v}, {1_v, 3_v}, {2_v, 3_v}});
  Index Idx = Builder().build(G, {0_v});

  checkIndex(G, {0_v}, Idx);
  EXPECT_FALSE(Idx.reaches(1_v, 2_v));
  EXPECT_FALSE(Idx.reaches(2_v, 1_v));
  EXPECT_TRUE(Idx.reaches(2_v, 3_v));
}

TEST(IntervalReachabilityTest, Cycle) {
  auto G = makeGraph(5, {{0_v, 1_v}, {1_v, 2_v}, {2_v, 0_v}, {2_v, 3_v}});
  Index Idx = Builder().build(G, {1_v});

  checkIndex(G, {1_v}, Idx);
  EXPECT_EQ(Idx.sccOf(0_v), Idx.sccOf(1_v));
  EXPECT_EQ(Idx.sccOf(1_v), Idx.sccOf(2_v));
  EXPECT_NE(Idx.sccOf(2_v), Idx.sccOf(3_v));
  EXPECT_FALSE(Idx.isIndexed(4_v));
  EXPECT_EQ(Idx.sccOf(4_v), Index::NoSCC);
}

TEST(IntervalReachabilityTest, SelfLoopAndMultiEdges) {
  auto G = makeGraph(3, {{0_v, 0_v}, {0_v, 1_v}, {0_v, 1_v}, {1_v, 2_v}});
  Index Idx = Builder().build(G, {0_v});

  checkIndex(G, {0_v}, Idx);
}

TEST(IntervalReachabilityTest, CrossEdgesIntoEarlierSubtrees) {
  // DFS from 0 visits 1 -> 2 first; 3 -> 2 and 4 -> 1 are cross edges
  auto G = makeGraph(6, {{0_v, 1_v},
                         {1_v, 2_v},
                         {0_v, 3_v},
                         {3_v, 2_v},
                         {3_v, 4_v},
                         {4_v, 1_v},
                         {4_v, 5_v}});
  Index Idx = Builder().build(G, {0_v});

  checkIndex(G, {0_v}, Idx);
  EXPECT_TRUE(Idx.reaches(3_v, 1_v));
  EXPECT_FALSE(Idx.reaches(1_v, 3_v));
}

TEST(IntervalReachabilityTest, MultipleRoots) {
  auto G = makeGraph(6, {{0_v, 1_v}, {2_v, 1_v}, {2_v, 3_v}, {4_v, 5_v}});
  Index Idx = Builder().build(G, {0_v, 2_v});

  checkIndex(G, {0_v, 2_v}, Idx);
  EXPECT_FALSE(Idx.isIndexed(4_v));
  EXPECT_FALSE(Idx.isIndexed(5_v));
}

TEST(IntervalReachabilityTest, ProjectedTargets) {
  auto G = makeGraph(
      6,
      {{0_v, 1_v}, {1_v, 2_v}, {0_v, 3_v}, {3_v, 4_v}, {4_v, 2_v}, {3_v, 5_v}});
  VtxSet IsTarget(6);
  IsTarget.insert(2_v);
  IsTarget.insert(5_v);

  Index Idx = Builder().build(
      G, {0_v}, [&](VtxId Vtx) { return IsTarget.contains(Vtx); });

  checkIndex(G, {0_v}, IsTarget, Idx);
  EXPECT_EQ(Idx.numTargets(), 2U);
  EXPECT_EQ(Idx.numIntervalsOf(Idx.sccOf(0_v)), 1U);
  EXPECT_EQ(Idx.numIntervalsOf(Idx.sccOf(1_v)), 1U);
}

TEST(IntervalReachabilityTest, EmptyGraph) {
  Graph G;
  Index Idx = Builder().build(G, {});

  EXPECT_EQ(Idx.numVertices(), 0U);
  EXPECT_EQ(Idx.numTargets(), 0U);
  EXPECT_EQ(Idx.numSCCs(), 0U);
}

Graph makeRandomGraph(std::mt19937 &Rng, size_t NumVertices, size_t NumEdges) {
  std::uniform_int_distribution<uint32_t> VtxDist(0, NumVertices - 1);
  llvm::SmallVector<Edge> Edges;
  Edges.reserve(NumEdges);
  for (size_t I = 0; I != NumEdges; ++I) {
    Edges.emplace_back(VtxId(VtxDist(Rng)), VtxId(VtxDist(Rng)));
  }
  return makeGraph(NumVertices, Edges);
}

TEST(IntervalReachabilityTest, RandomGraphs) {
  std::mt19937 Rng(42); // NOLINT(cert-msc32-c,cert-msc51-cpp)
  std::bernoulli_distribution IsTargetDist(0.3);

  // Reuse builder and index across graphs to check that no state leaks
  Builder B;
  Index Idx;

  for (size_t NumVertices : {1, 2, 5, 16, 50}) {
    for (double EdgeFactor : {0.5, 1.0, 1.5, 3.0}) {
      for (int Iteration = 0; Iteration != 8; ++Iteration) {
        auto G = makeRandomGraph(Rng, NumVertices,
                                 size_t(double(NumVertices) * EdgeFactor));

        llvm::SmallVector<VtxId> Roots{VtxId(0)};
        if (NumVertices > 2) {
          Roots.push_back(VtxId(NumVertices / 2));
        }

        SCOPED_TRACE("NumVertices=" + std::to_string(NumVertices) +
                     ", EdgeFactor=" + std::to_string(EdgeFactor) +
                     ", Iteration=" + std::to_string(Iteration));

        B.build(G, Roots, Idx);
        checkIndex(G, Roots, Idx);

        VtxSet IsTarget(NumVertices);
        for (auto Vtx : psr::iota<VtxId>(NumVertices)) {
          if (IsTargetDist(Rng)) {
            IsTarget.insert(Vtx);
          }
        }
        B.build(G, Roots, Idx,
                [&](VtxId Vtx) { return IsTarget.contains(Vtx); });
        checkIndex(G, Roots, IsTarget, Idx);

        auto Rev = Graph::reversed(G);
        B.build(Rev, Roots, Idx);
        checkIndex(Rev, Roots, Idx);

        if (::testing::Test::HasFailure()) {
          return;
        }
      }
    }
  }
}

} // namespace

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
