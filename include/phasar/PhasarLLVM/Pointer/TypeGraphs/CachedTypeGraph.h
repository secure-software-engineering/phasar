/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * CachedTypeGraph.h
 *
 *  Created on: 28.06.2018
 *      Author: nicolas bellec
 */

#ifndef PHASAR_PHASARLLVM_POINTER_TYPEGRAPHS_CACHEDTYPEGRAPH_H_
#define PHASAR_PHASARLLVM_POINTER_TYPEGRAPHS_CACHEDTYPEGRAPH_H_

#include "phasar/PhasarLLVM/Pointer/TypeGraphs/TypeGraph.h"

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/graph_traits.hpp"
#include "boost/graph/reverse_graph.hpp"
#include "gtest/gtest_prod.h"

#include <set>
#include <string>
#include <unordered_map>

namespace llvm {
class StructType;
} // namespace llvm

namespace psr {
class CachedTypeGraph : public TypeGraph<CachedTypeGraph> {
protected:
  struct VertexProperties {
    std::string Name;
    std::set<const llvm::StructType *> Types;
    const llvm::StructType *BaseType = nullptr;
  };

  struct EdgeProperties {
    EdgeProperties() = default;
  };

  using graph_t =
      boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS,
                            VertexProperties, EdgeProperties>;

  using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;
  using vertex_iterator = boost::graph_traits<graph_t>::vertex_iterator;
  using edge_t = boost::graph_traits<graph_t>::edge_descriptor;
  using out_edge_iterator = boost::graph_traits<graph_t>::out_edge_iterator;
  using in_edge_iterator = boost::graph_traits<graph_t>::in_edge_iterator;

  using rev_graph_t = boost::reverse_graph<graph_t, graph_t &>;

  using rev_vertex_t = boost::graph_traits<rev_graph_t>::vertex_descriptor;
  using rev_vertex_iterator = boost::graph_traits<rev_graph_t>::vertex_iterator;
  using rev_edge_t = boost::graph_traits<rev_graph_t>::edge_descriptor;
  using rev_out_edge_iterator =
      boost::graph_traits<rev_graph_t>::out_edge_iterator;
  using rev_in_edge_iterator =
      boost::graph_traits<rev_graph_t>::in_edge_iterator;

  struct dfs_visitor;
  struct reverse_type_propagation_dfs_visitor;

  std::unordered_map<std::string, vertex_t> TypeVertexMap;
  graph_t G;
  bool AlreadyVisited = false;

  FRIEND_TEST(TypeGraphTest, AddType);
  FRIEND_TEST(TypeGraphTest, AddLinkSimple);
  FRIEND_TEST(TypeGraphTest, AddLinkWithSubs);
  FRIEND_TEST(TypeGraphTest, AddLinkWithRecursion);
  FRIEND_TEST(TypeGraphTest, ReverseTypePropagation);
  FRIEND_TEST(TypeGraphTest, TypeAggregation);
  FRIEND_TEST(TypeGraphTest, Merging);

  vertex_t addType(const llvm::StructType *NewType);
  void reverseTypePropagation(const llvm::StructType *BaseStruct);
  void aggregateTypes();
  bool addLinkWithoutReversePropagation(const llvm::StructType *From,
                                        const llvm::StructType *To);

public:
  CachedTypeGraph() = default;

  ~CachedTypeGraph() override = default;
  // CachedTypeGraph(const CachedTypeGraph &copy) = delete;
  // CachedTypeGraph& operator=(const CachedTypeGraph &copy) = delete;
  // CachedTypeGraph(CachedTypeGraph &&move) = delete;
  // CachedTypeGraph& operator=(CachedTypeGraph &&move) = delete;

  bool addLink(const llvm::StructType *From,
               const llvm::StructType *To) override;
  void printAsDot(const std::string &Path = "typegraph.dot") const override;
  std::set<const llvm::StructType *>
  getTypes(const llvm::StructType *StructType) override;
};
} // namespace psr

#endif
