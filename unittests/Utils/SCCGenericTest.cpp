/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and other
 *****************************************************************************/

#include "phasar/Utils/SCCGeneric.h"

#include "phasar/Utils/AdjacencyList.h"
#include "phasar/Utils/EmptyBaseOptimizationUtils.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/ArrayRef.h"

#include "gtest/gtest.h"

#include <type_traits>

namespace {
using namespace psr;

enum class NodeId : uint32_t {};

using ExampleGraph = AdjacencyList<EmptyType, NodeId>;

static SCCHolder<NodeId> makeGTSCCs(llvm::ArrayRef<std::set<int>> SCCs) {
  SCCHolder<NodeId> Ret;

  uint32_t Ctr = 0;
  for (const auto &SCC : SCCs) {
    auto CurrSCC = SCCId<NodeId>(Ctr++);
    auto &NodesInSCC = Ret.NodesInSCC.emplace_back();
    for (auto Nod : SCC) {
      NodesInSCC.push_back(NodeId(Nod));

      if (Ret.SCCOfNode.size() <= size_t(Nod)) {
        Ret.SCCOfNode.resize(Nod + 1);
      }

      Ret.SCCOfNode[NodeId(Nod)] = CurrSCC;
    }
  }

  return Ret;
};

static void compareSCCs(const SCCHolder<NodeId> &ComputedSCCs,
                        const SCCHolder<NodeId> &ExpectedSCCs,
                        std::string_view ComputedName) {
  ASSERT_EQ(ComputedSCCs.size(), ExpectedSCCs.size())
      << "Unequal number of SCC components\n";
  ASSERT_EQ(ComputedSCCs.SCCOfNode.size(), ExpectedSCCs.SCCOfNode.size())
      << "Unequal number of Graph Nodes\n";

  const auto None = SCCId<NodeId>(UINT32_MAX);
  TypedVector<SCCId<NodeId>, SCCId<NodeId>> Isomorphism(ComputedSCCs.size(),
                                                        None);

  for (auto Vtx : iota<NodeId>(ComputedSCCs.SCCOfNode.size())) {
    auto ExpectedSCC = ExpectedSCCs.SCCOfNode[Vtx];
    auto ComputedSCC = ComputedSCCs.SCCOfNode[Vtx];

    if (Isomorphism[ExpectedSCC] == None) {
      Isomorphism[ExpectedSCC] = ComputedSCC;
    } else {
      EXPECT_EQ(Isomorphism[ExpectedSCC], ComputedSCC)
          << "SCCs differ for node: " << uint32_t(Vtx) << " in "
          << ComputedName;
    }
  }
}

static void validateTopologicalOrder(const ExampleGraph &Graph,
                                     const SCCHolder<NodeId> &ComputedSCCs,
                                     std::string_view ComputedName) {
  // Note: Pearce's algorithm produces SCCs in reverse-topological order
  for (auto [Vtx, SCC] : ComputedSCCs.SCCOfNode.enumerate()) {
    for (auto Succ : Graph.Adj[Vtx]) {
      auto SuccSCC = ComputedSCCs.SCCOfNode[Succ];
      EXPECT_LE(+SuccSCC, +SCC)
          << "Invalid topological order in " << ComputedName << ": SCC #"
          << +SCC << " must come before #" << +SuccSCC;
    }
  }
}

static void computeSCCsAndCompare(ExampleGraph &Graph,
                                  llvm::ArrayRef<std::set<int>> ExpectedSCCs) {

  auto ComputedSCCsIt = computeSCCs(Graph);
  auto ComputedSCCsRec = computeSCCs(Graph, std::false_type{});
  ASSERT_EQ(ComputedSCCsIt.SCCOfNode.size(), Graph.Adj.size())
      << "Iterative Pearce's Approach did not reach all nodes\n";
  ASSERT_EQ(ComputedSCCsIt.SCCOfNode.size(), Graph.Adj.size())
      << "Recursive Pearce's Approach did not reach all nodes\n";

  [[maybe_unused]] auto SCCDeps = computeSCCDependencies(Graph, ComputedSCCsIt);
#if __cplusplus >= 202002L
  static_assert(is_const_graph<decltype(SCCDeps)>);
#endif

  auto GroundTruth = makeGTSCCs(ExpectedSCCs);
  compareSCCs(ComputedSCCsIt, GroundTruth, "Pearce Iterative");
  validateTopologicalOrder(Graph, ComputedSCCsIt, "Pearce Iterative");
  if (::testing::Test::HasFailure()) {
    ComputedSCCsIt.print(Graph, llvm::outs(), "ExampleGraph");
    return;
  }

  compareSCCs(ComputedSCCsRec, GroundTruth, "Pearce Recursive");
  validateTopologicalOrder(Graph, ComputedSCCsRec, "Pearce Recursive");
  if (::testing::Test::HasFailure()) {
    ComputedSCCsRec.print(Graph, llvm::outs(), "ExampleGraph");
  }
}

TEST(SCCGenericTest, SCCTest01) {
  ExampleGraph Graph{{{NodeId(2)},
                      {NodeId(0)},
                      {NodeId(1)},
                      {NodeId(1), NodeId(2)},
                      {NodeId(1)},
                      {NodeId(4), NodeId(6)},
                      {NodeId(4), NodeId(7)},
                      {NodeId(5)}}};
  computeSCCsAndCompare(Graph, {{0, 1, 2}, {3}, {4}, {5, 6, 7}});
}

TEST(SCCGenericTest, SCCTest02) {
  ExampleGraph Graph{{{}, {}, {}, {}, {}, {}, {}, {}, {}, {}}};
  computeSCCsAndCompare(Graph,
                        {{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}});
}

TEST(SCCGenericTest, SCCTest03) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(3)},
                      {NodeId(4)},
                      {NodeId(5)},
                      {NodeId(6)},
                      {NodeId(0)}}};
  computeSCCsAndCompare(Graph, {{0, 1, 2, 3, 4, 5, 6}});
}

TEST(SCCGenericTest, SCCTest04) {
  ExampleGraph Graph{{{NodeId(1), NodeId(2), NodeId(3), NodeId(4)},
                      {NodeId(0), NodeId(2), NodeId(3), NodeId(4)},
                      {NodeId(0), NodeId(1), NodeId(3), NodeId(4)},
                      {NodeId(0), NodeId(1), NodeId(2), NodeId(4)},
                      {NodeId(0), NodeId(1), NodeId(2), NodeId(3)}}};
  computeSCCsAndCompare(Graph, {{0, 1, 2, 3, 4}});
}

TEST(SCCGenericTest, SCCTest05) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(3), NodeId(4)},
                      {NodeId(5)},
                      {NodeId(5)},
                      {NodeId(2), NodeId(6)},
                      {NodeId(7)},
                      {NodeId(1), NodeId(8)},
                      {}}};
  computeSCCsAndCompare(Graph, {{0}, {1, 2, 3, 4, 5, 6, 7}, {8}});
}

TEST(SCCGenericTest, SCCTest06) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(3)},
                      {NodeId(4)},
                      {NodeId(5)},
                      {NodeId(6)},
                      {NodeId(7)},
                      {NodeId(0)},
                      {NodeId(9)},
                      {NodeId(10)},
                      {NodeId(11)},
                      {NodeId(12)},
                      {NodeId(13), NodeId(4)},
                      {NodeId(8)},
                      {NodeId(9)},
                      {NodeId(3)},
                      {NodeId(5)}}};
  computeSCCsAndCompare(
      Graph,
      {{0, 1, 2, 3, 4, 5, 6, 7}, {8, 9, 10, 11, 12, 13}, {14}, {15}, {16}});
}

// Note: Following tests generated by ChatGPT

// SCC test: two disjoint cycles
TEST(SCCGenericTest, SCCTest07) {
  ExampleGraph Graph{{{NodeId(1)}, {NodeId(0)}, {NodeId(3)}, {NodeId(2)}}};
  computeSCCsAndCompare(Graph, {{0, 1}, {2, 3}});
}

// SCC test: diamond shape, no cycles
TEST(SCCGenericTest, SCCTest08) {
  ExampleGraph Graph{{{NodeId(1), NodeId(2)}, {NodeId(3)}, {NodeId(3)}, {}}};
  computeSCCsAndCompare(Graph, {{0}, {1}, {2}, {3}});
}

// SCC test: diamond with back edge creating cycle
TEST(SCCGenericTest, SCCTest09) {
  ExampleGraph Graph{
      {{NodeId(1), NodeId(2)}, {NodeId(3)}, {NodeId(3)}, {NodeId(0)}}};
  computeSCCsAndCompare(Graph, {{0, 1, 2, 3}});
}

// SCC test: one self-loop, others acyclic
TEST(SCCGenericTest, SCCTest10) {
  ExampleGraph Graph{{{NodeId(0)}, {NodeId(2)}, {}}};
  computeSCCsAndCompare(Graph, {{0}, {1}, {2}});
}

// SCC test: disconnected nodes
TEST(SCCGenericTest, SCCTest11) {
  ExampleGraph Graph{{{}, {}, {}}};
  computeSCCsAndCompare(Graph, {{0}, {1}, {2}});
}

// SCC test: complex graph with two larger SCCs and one singleton
TEST(SCCGenericTest, SCCTest12) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(0)}, // cycle 0-1-2
                      {NodeId(4)},
                      {NodeId(5)},
                      {NodeId(3)}, // cycle 3-4-5
                      {}}};
  computeSCCsAndCompare(Graph, {{0, 1, 2}, {3, 4, 5}, {6}});
}

// SCC test: nested cycles sharing a node
TEST(SCCGenericTest, SCCTest13) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(0), NodeId(3)},
                      {NodeId(4)},
                      {NodeId(2)}}};
  // 0-1-2 form a cycle, and 2-3-4 also cycle back to 2 => all {0,1,2,3,4}
  computeSCCsAndCompare(Graph, {{0, 1, 2, 3, 4}});
}

// SCC test: long linear chain ending in a self-loop
TEST(SCCGenericTest, SCCTest14) {
  ExampleGraph Graph{{{NodeId(1)}, {NodeId(2)}, {NodeId(3)}, {NodeId(3)}}};
  // nodes 0,1,2 feed into 3; node 3 has self-loop
  computeSCCsAndCompare(Graph, {{0}, {1}, {2}, {3}});
}

// SCC test: three SCCs connected in DAG shape
TEST(SCCGenericTest, SCCTest15) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(0)}, // SCC {0,1}
                      {NodeId(3)},
                      {NodeId(2)}, // SCC {2,3}
                      {NodeId(5)},
                      {NodeId(4)}}}; // SCC {4,5}
  computeSCCsAndCompare(Graph, {{0, 1}, {2, 3}, {4, 5}});
}

// SCC test: two big SCCs connected by single edge
TEST(SCCGenericTest, SCCTest16) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(0)}, // cycle 0-1-2
                      {NodeId(4)},
                      {NodeId(5)},
                      {NodeId(3), NodeId(0)}}}; // cycle 3-4-5, with edge 5->0
  // Two SCCs {0,1,2} and {3,4,5}; edge {3,4,5} -> {0,1,2}
  computeSCCsAndCompare(Graph, {{0, 1, 2}, {3, 4, 5}});
}

// SCC test: large cycle with attached tail
TEST(SCCGenericTest, SCCTest17) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(3)},
                      {NodeId(4)},
                      {NodeId(0)},   // cycle 0-1-2-3-4-0
                      {NodeId(0)}}}; // tail node 5 -> 0
  computeSCCsAndCompare(Graph, {{0, 1, 2, 3, 4}, {5}});
}

// SCC test: two SCCs joined by a “bow-tie” structure
TEST(SCCGenericTest, SCCTest18) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(0)}, // left cycle {0,1,2}
                      {NodeId(4)},
                      {NodeId(5)},
                      {NodeId(3)},              // right cycle {3,4,5}
                      {NodeId(0), NodeId(3)}}}; // node 6 links both
  computeSCCsAndCompare(Graph, {{0, 1, 2}, {3, 4, 5}, {6}});
}

// SCC test: complete bipartite between {0,1} and {2,3}
TEST(SCCGenericTest, SCCTest19) {
  ExampleGraph Graph{{{NodeId(2), NodeId(3)},
                      {NodeId(2), NodeId(3)},
                      {NodeId(0), NodeId(1)},
                      {NodeId(0), NodeId(1)}}};
  // All nodes strongly connected
  computeSCCsAndCompare(Graph, {{0, 1, 2, 3}});
}

// SCC test: three SCCs connected linearly
TEST(SCCGenericTest, SCCTest20) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(0)}, // cycle {0,1,2}
                      {NodeId(4)},
                      {NodeId(3)}, // cycle {3,4}
                      {NodeId(6)},
                      {NodeId(5)}}}; // cycle {5,6}
  computeSCCsAndCompare(Graph, {{0, 1, 2}, {3, 4}, {5, 6}});
}

// SCC test: complex graph with interleaved cycles
TEST(SCCGenericTest, SCCTest21) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(0)}, // cycle {0,1,2}
                      {NodeId(1), NodeId(4)},
                      {NodeId(5)},
                      {NodeId(3)}, // cycle {3,4,5}
                      {NodeId(7)},
                      {NodeId(6)}}}; // cycle {6,7}
  // SCCs: {0,1,2}, {3,4,5}, {6,7}
  computeSCCsAndCompare(Graph, {{0, 1, 2}, {3, 4, 5}, {6, 7}});
}

// SCC test: “ladder” structure with rungs forming cycles
TEST(SCCGenericTest, SCCTest22) {
  ExampleGraph Graph{{{NodeId(1), NodeId(2)},
                      {NodeId(0), NodeId(3)},
                      {NodeId(0), NodeId(3)},
                      {NodeId(1), NodeId(2)}}};
  // Essentially two squares connected; all nodes mutually reachable
  computeSCCsAndCompare(Graph, {{0, 1, 2, 3}});
}

// SCC test: disconnected large SCCs plus singletons
TEST(SCCGenericTest, SCCTest23) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(0)}, // SCC {0,1,2}
                      {NodeId(4)},
                      {NodeId(3)}, // SCC {3,4}
                      {},
                      {}, // nodes 5,6 isolated
                      {NodeId(9)},
                      {NodeId(9)},
                      {NodeId(8)}}}; // SCC {8,9}
  computeSCCsAndCompare(Graph, {{0, 1, 2}, {3, 4}, {5}, {6}, {7}, {8, 9}});
}

// SCC test: 12-node graph with 4 SCCs, each of size 3
TEST(SCCGenericTest, SCCTest24) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(0)}, // {0,1,2}
                      {NodeId(4)},
                      {NodeId(5)},
                      {NodeId(3)}, // {3,4,5}
                      {NodeId(7)},
                      {NodeId(8)},
                      {NodeId(6)}, // {6,7,8}
                      {NodeId(10)},
                      {NodeId(11)},
                      {NodeId(9)}}}; // {9,10,11}
  computeSCCsAndCompare(Graph, {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {9, 10, 11}});
}

// SCC test: 15-node graph with one large SCC and dangling tails
TEST(SCCGenericTest, SCCTest25) {
  ExampleGraph Graph{{
      {NodeId(1)},
      {NodeId(2)},
      {NodeId(3)},
      {NodeId(0)}, // {0,1,2,3}
      {NodeId(5)},
      {NodeId(4)}, // {4,5}
      {NodeId(7)},
      {NodeId(8)},
      {NodeId(6)}, // {6,7,8}
      {NodeId(0)},
      {NodeId(4)},
      {NodeId(6)}, // tails into SCCs
      {},
      {},
      {} // 3 isolated
  }};
  computeSCCsAndCompare(
      Graph,
      {{0, 1, 2, 3}, {4, 5}, {6, 7, 8}, {9}, {10}, {11}, {12}, {13}, {14}});
}

// SCC test: 16-node graph with interlinked clusters
TEST(SCCGenericTest, SCCTest26) {
  ExampleGraph Graph{
      {{NodeId(1)},
       {NodeId(2)},
       {NodeId(0)}, // {0,1,2}
       {NodeId(4)},
       {NodeId(5)},
       {NodeId(3)}, // {3,4,5}
       {NodeId(7)},
       {NodeId(6)}, // {6,7}
       {NodeId(9)},
       {NodeId(10)},
       {NodeId(8)}, // {8,9,10}
       {NodeId(12)},
       {NodeId(11)},                                 // {11,12}
       {NodeId(0), NodeId(3), NodeId(6), NodeId(8)}, // 13 links clusters
       {NodeId(13)},                                 // 14 -> 13
       {NodeId(14)}}};                               // 15 -> 14 -> 13
  computeSCCsAndCompare(
      Graph,
      {{0, 1, 2}, {3, 4, 5}, {6, 7}, {8, 9, 10}, {11, 12}, {13}, {14}, {15}});
}

// SCC test: 18-node graph forming a big cycle plus smaller SCCs
TEST(SCCGenericTest, SCCTest27) {
  ExampleGraph Graph{{{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(3)},
                      {NodeId(4)},
                      {NodeId(5)},
                      {NodeId(6)},
                      {NodeId(7)},
                      {NodeId(8)},
                      {NodeId(9)},
                      {NodeId(10)},
                      {NodeId(11)},
                      {NodeId(0)}, // 0-11 cycle
                      {NodeId(13)},
                      {NodeId(12)}, // {12,13}
                      {NodeId(15)},
                      {NodeId(14)}, // {14,15}
                      {NodeId(17)},
                      {NodeId(16)}}}; // {16,17}
  computeSCCsAndCompare(
      Graph,
      {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, {12, 13}, {14, 15}, {16, 17}});
}

// SCC test: 20-node graph with mixed SCC sizes
TEST(SCCGenericTest, SCCTest28) {
  ExampleGraph Graph{{{NodeId(1)},  {NodeId(2)},
                      {NodeId(0)}, // {0,1,2}
                      {NodeId(4)},  {NodeId(5)},
                      {NodeId(3)},               // {3,4,5}
                      {NodeId(7)},  {NodeId(6)}, // {6,7}
                      {NodeId(9)},  {NodeId(8)}, // {8,9}
                      {NodeId(10)}, {NodeId(11)},
                      {NodeId(10)}, {NodeId(12)},
                      {NodeId(15)}, {NodeId(14)}, // {14,15}
                      {NodeId(17)}, {NodeId(18)},
                      {NodeId(19)}, {}}}; // chain 16->17->18->19->isolated
  computeSCCsAndCompare(Graph, {{0, 1, 2},
                                {3, 4, 5},
                                {6, 7},
                                {8, 9},
                                {10},
                                {11},
                                {12},
                                {13},
                                {14, 15},
                                {16},
                                {17},
                                {18},
                                {19}});
}

// SCC test: 25-node graph, 5 clusters of 5 nodes each forming cycles
TEST(SCCGenericTest, SCCTest29) {
  ExampleGraph Graph{// Cluster 0: nodes 0-4 cycle
                     {{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(3)},
                      {NodeId(4)},
                      {NodeId(0)},
                      // Cluster 1: nodes 5-9 cycle
                      {NodeId(6)},
                      {NodeId(7)},
                      {NodeId(8)},
                      {NodeId(9)},
                      {NodeId(5)},
                      // Cluster 2: nodes 10-14 cycle
                      {NodeId(11)},
                      {NodeId(12)},
                      {NodeId(13)},
                      {NodeId(14)},
                      {NodeId(10)},
                      // Cluster 3: nodes 15-19 cycle
                      {NodeId(16)},
                      {NodeId(17)},
                      {NodeId(18)},
                      {NodeId(19)},
                      {NodeId(15)},
                      // Cluster 4: nodes 20-24 cycle
                      {NodeId(21)},
                      {NodeId(22)},
                      {NodeId(23)},
                      {NodeId(24)},
                      {NodeId(20)}}};

  computeSCCsAndCompare(Graph, {{0, 1, 2, 3, 4},
                                {5, 6, 7, 8, 9},
                                {10, 11, 12, 13, 14},
                                {15, 16, 17, 18, 19},
                                {20, 21, 22, 23, 24}});
}

// SCC test: 25-node graph, one giant SCC (0-19 cycle) plus 5 isolated nodes
TEST(SCCGenericTest, SCCTest30) {
  ExampleGraph Graph{// Giant cycle through 0..19
                     {{NodeId(1)},
                      {NodeId(2)},
                      {NodeId(3)},
                      {NodeId(4)},
                      {NodeId(5)},
                      {NodeId(6)},
                      {NodeId(7)},
                      {NodeId(8)},
                      {NodeId(9)},
                      {NodeId(10)},
                      {NodeId(11)},
                      {NodeId(12)},
                      {NodeId(13)},
                      {NodeId(14)},
                      {NodeId(15)},
                      {NodeId(16)},
                      {NodeId(17)},
                      {NodeId(18)},
                      {NodeId(19)},
                      {NodeId(0)},
                      // Isolated nodes 20-24
                      {},
                      {},
                      {},
                      {},
                      {}}};

  computeSCCsAndCompare(Graph, {{0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
                                 10, 11, 12, 13, 14, 15, 16, 17, 18, 19},
                                {20},
                                {21},
                                {22},
                                {23},
                                {24}});
}

// SCC test: 25-node graph, 5 SCC clusters of size 5, linked in DAG
TEST(SCCGenericTest, SCCTest31) {
  ExampleGraph Graph{
      {// 0..24
       /* 0 */ {
           NodeId(1),
           NodeId(5)}, // cycle 0->1->2->3->4->0 and cross 0->5 (to cluster1)
       /* 1 */ {NodeId(2)},
       /* 2 */ {NodeId(3)},
       /* 3 */ {NodeId(4)},
       /* 4 */ {NodeId(0)},
       /* 5 */ {NodeId(6)},             // cluster1
       /* 6 */ {NodeId(7), NodeId(10)}, // 6->7 and cross 6->10 (to cluster2)
       /* 7 */ {NodeId(8)},
       /* 8 */ {NodeId(9)},
       /* 9 */ {NodeId(5)},
       /*10 */ {NodeId(11)}, // cluster2
       /*11 */ {NodeId(12)},
       /*12 */ {NodeId(13), NodeId(15)}, // 12->13 and cross 12->15 (to
                                         // cluster3)
       /*13 */ {NodeId(14)},
       /*14 */ {NodeId(10)},
       /*15 */ {NodeId(16)}, // cluster3
       /*16 */ {NodeId(17)},
       /*17 */ {NodeId(18), NodeId(20)}, // 17->18 and cross 17->20 (to
                                         // cluster4)
       /*18 */ {NodeId(19)},
       /*19 */ {NodeId(15)},
       /*20 */ {NodeId(21)}, // cluster4
       /*21 */ {NodeId(22)},
       /*22 */ {NodeId(23)},
       /*23 */ {NodeId(24)},
       /*24 */ {NodeId(20)}}};

  computeSCCsAndCompare(Graph, {{0, 1, 2, 3, 4},
                                {5, 6, 7, 8, 9},
                                {10, 11, 12, 13, 14},
                                {15, 16, 17, 18, 19},
                                {20, 21, 22, 23, 24}});
}

// SCC test: 25-node graph, one giant SCC (0-19 cycle) plus 5 isolated nodes,
// with edges from the big SCC to the isolated nodes
TEST(SCCGenericTest, SCCTest32) {
  ExampleGraph Graph{
      {// 0..24
       /* 0 */ {NodeId(1),
                NodeId(20)}, // cycle 0->1->...->19->0 and extra 0->20
       /* 1 */ {NodeId(2)},
       /* 2 */ {NodeId(3)},
       /* 3 */ {NodeId(4)},
       /* 4 */ {NodeId(5)},
       /* 5 */ {NodeId(6), NodeId(21)}, // 5->6 and extra 5->21
       /* 6 */ {NodeId(7)},
       /* 7 */ {NodeId(8)},
       /* 8 */ {NodeId(9)},
       /* 9 */ {NodeId(10)},
       /*10 */ {NodeId(11), NodeId(22)}, // 10->11 and extra 10->22
       /*11 */ {NodeId(12)},
       /*12 */ {NodeId(13)},
       /*13 */ {NodeId(14)},
       /*14 */ {NodeId(15)},
       /*15 */ {NodeId(16), NodeId(23)}, // 15->16 and extra 15->23
       /*16 */ {NodeId(17)},
       /*17 */ {NodeId(18)},
       /*18 */ {NodeId(19)},
       /*19 */ {NodeId(0), NodeId(24)}, // 19->0 and extra 19->24
       /*20 */ {},                      // isolated singletons
       /*21 */ {},
       /*22 */ {},
       /*23 */ {},
       /*24 */ {}}};

  computeSCCsAndCompare(Graph, {{0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
                                 10, 11, 12, 13, 14, 15, 16, 17, 18, 19},
                                {20},
                                {21},
                                {22},
                                {23},
                                {24}});
}

} // namespace

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
