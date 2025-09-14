/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and other
 *****************************************************************************/

#include "phasar/Utils/SCCGeneric.h"

#include "phasar/Utils/AdjacencyList.h"
#include "phasar/Utils/EmptyBaseOptimizationUtils.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/ArrayRef.h"

#include "gtest/gtest.h"

#include <cstdint>

//===----------------------------------------------------------------------===//
// Unit tests for the generic SCC algorithm

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

static void computeSCCsAndCompare(ExampleGraph &Graph,
                                  llvm::ArrayRef<std::set<int>> ExpectedSCCs) {

  auto OutputRec = computeSCCs(Graph);
  auto OutputIt = computeSCCsIterative(Graph);
  ASSERT_EQ(OutputIt.SCCOfNode.size(), Graph.Adj.size())
      << "Iterative Approach did not reach all nodes\n";
  ASSERT_EQ(OutputRec.SCCOfNode.size(), Graph.Adj.size())
      << "Recursive Approach did not reach all nodes\n";

#if __cplusplus >= 202002L
  [[maybe_unused]] auto SCCDeps = computeSCCDependencies(Graph, OutputRec);
  static_assert(is_const_graph<decltype(SCCDeps)>);
#endif

  auto GroundTruth = makeGTSCCs(ExpectedSCCs);
  compareSCCs(OutputRec, GroundTruth, "RecursiveTarjan");
  compareSCCs(OutputIt, GroundTruth, "IterativeTarjan");

  // printGraph(Graph, llvm::outs(), "ExampleGraph");
  OutputRec.print(Graph, llvm::outs(), "ExampleGraph");
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

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
