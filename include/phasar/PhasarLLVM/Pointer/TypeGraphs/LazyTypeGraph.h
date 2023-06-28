/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LazyTypeGraph.h
 *
 *  Created on: 28.06.2018
 *      Author: nicolas bellec
 */

#ifndef PHASAR_PHASARLLVM_POINTER_TYPEGRAPHS_LAZYTYPEGRAPH_H
#define PHASAR_PHASARLLVM_POINTER_TYPEGRAPHS_LAZYTYPEGRAPH_H

#include "phasar/PhasarLLVM/Pointer/TypeGraphs/TypeGraph.h"

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/graph_traits.hpp"
#include "boost/graph/reverse_graph.hpp"

#include <set>
#include <string>
#include <unordered_map>

namespace llvm {
class StructType;
} // namespace llvm

namespace psr {
class LazyTypeGraph : public TypeGraph<LazyTypeGraph> {
protected:
  struct VertexProperties {
    std::string Name;
    const llvm::StructType *SType = nullptr;
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

  std::unordered_map<std::string, vertex_t> TypeToVertexMap;
  graph_t Graph;
  bool AlreadyVisited = false;

  vertex_t addType(const llvm::StructType *NewType);
  void aggregateTypes();

public:
  LazyTypeGraph() = default;

  ~LazyTypeGraph() override = default;
  // LazyTypeGraph(const LazyTypeGraph &copy) = delete;
  // LazyTypeGraph& operator=(const LazyTypeGraph &copy) = delete;
  // LazyTypeGraph(LazyTypeGraph &&move) = delete;
  // LazyTypeGraph& operator=(LazyTypeGraph &&move) = delete;

  [[nodiscard]] bool addLink(const llvm::StructType *From,
                             const llvm::StructType *To) override;
  void printAsDot(const std::string &Path = "typegraph.dot") const override;
  [[nodiscard]] std::set<const llvm::StructType *>
  getTypes(const llvm::StructType *StructType) override;
};
} // namespace psr

#endif
