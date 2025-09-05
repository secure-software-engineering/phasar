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
#include "phasar/Utils/TypedVector.h"

#include "gtest/gtest.h"

#include <cstdint>

//===----------------------------------------------------------------------===//
// Unit tests for the Igeneric SCC algorithm

using namespace psr;

enum class NodeId : uint32_t {};

using ExampleGraph = AdjacencyList<EmptyType, NodeId>;

static void computeSCCsAndCompare(ExampleGraph &Graph) {
  auto OutputRec = computeSCCs(Graph);
  auto OutputIt = computeSCCIterative(Graph);
  ASSERT_EQ(OutputIt.SCCOfNode.size(), Graph.Adj.size())
      << "Iterative Approach did not reach all nodes\n";
  ASSERT_EQ(OutputRec.SCCOfNode.size(), Graph.Adj.size())
      << "Recursive Approach did not reach all nodes\n";
  ASSERT_EQ(OutputRec.size(), OutputIt.size())
      << "Unequal number of SCC components\n";

  const auto None = SCCId<NodeId>(UINT32_MAX);
  TypedVector<SCCId<NodeId>, SCCId<NodeId>> Isomorphism(OutputRec.size(), None);

  for (auto Vtx : GraphTraits<ExampleGraph>::vertices(Graph)) {
    auto RecSCC = OutputRec.SCCOfNode[Vtx];
    auto ItSCC = OutputIt.SCCOfNode[Vtx];

    if (Isomorphism[RecSCC] == None) {
      Isomorphism[RecSCC] = ItSCC;
    } else {
      EXPECT_EQ(Isomorphism[RecSCC], ItSCC)
          << "SCCs differ at Index: " << uint32_t(Vtx) << "\n";
    }
  }

#if __cplusplus >= 202002L
  auto SCCDeps = computeSCCDependencies(Graph, OutputRec);
  static_assert(is_const_graph<decltype(SCCDeps)>);
#endif
}

TEST(SCCGenericTest, SCCTest) {
  ExampleGraph GraphOne{{{NodeId(2)},
                         {NodeId(0)},
                         {NodeId(1)},
                         {NodeId(1), NodeId(2)},
                         {NodeId(1)},
                         {NodeId(4), NodeId(6)},
                         {NodeId(4), NodeId(7)},
                         {NodeId(5)}}};

  ExampleGraph GraphTwo{{{}, {}, {}, {}, {}, {}, {}, {}, {}, {}}};

  ExampleGraph GraphThree{{{NodeId(1)},
                           {NodeId(2)},
                           {NodeId(3)},
                           {NodeId(4)},
                           {NodeId(5)},
                           {NodeId(6)},
                           {NodeId(0)}}};

  ExampleGraph GraphFour{{{NodeId(1), NodeId(2), NodeId(3), NodeId(4)},
                          {NodeId(0), NodeId(2), NodeId(3), NodeId(4)},
                          {NodeId(0), NodeId(1), NodeId(3), NodeId(4)},
                          {NodeId(0), NodeId(1), NodeId(2), NodeId(4)},
                          {NodeId(0), NodeId(1), NodeId(2), NodeId(3)}}};

  ExampleGraph GraphFive{{{NodeId(1)},
                          {NodeId(2)},
                          {NodeId(3), NodeId(4)},
                          {NodeId(5)},
                          {NodeId(5)},
                          {NodeId(2), NodeId(6)},
                          {NodeId(7)},
                          {NodeId(1), NodeId(8)},
                          {}}};

  ExampleGraph GraphSix{{{NodeId(1)},
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

  std::vector<ExampleGraph> TestGraphs = {GraphOne,  GraphTwo,  GraphThree,
                                          GraphFour, GraphFive, GraphSix};

  for (auto &TestGraph : TestGraphs) {
    computeSCCsAndCompare(TestGraph);
  }
}

// main function for the test case
int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
