/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * TypeGraph.cpp
 *
 *  Created on: 28.06.2018
 *      Author: nicolas bellec
 */

#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/graph_utility.hpp"
#include "boost/graph/graphviz.hpp"
#include "boost/property_map/dynamic_property_map.hpp"

#include "llvm/IR/DerivedTypes.h"

#include "phasar/PhasarLLVM/Pointer/TypeGraphs/CachedTypeGraph.h"

#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;

namespace psr {

struct CachedTypeGraph::dfs_visitor : public boost::default_dfs_visitor {
  dfs_visitor(graph_t *G) : G(G) {}

  void finish_edge(edge_t E, graph_t const &U) { // NOLINT
    CachedTypeGraph::vertex_t Src = boost::source(E, U);
    CachedTypeGraph::vertex_t Target = boost::target(E, U);

    for (const auto *TargetType : U[Target].types) {
      (*G)[Src].types.insert(TargetType);
    }
  }

  graph_t *G;
};

struct CachedTypeGraph::reverse_type_propagation_dfs_visitor
    : public boost::default_dfs_visitor {
  reverse_type_propagation_dfs_visitor(rev_graph_t *G) : G(G) {}

  void examine_edge(rev_edge_t E, rev_graph_t const &U) { // NOLINT
    auto Src = boost::source(E, U);
    auto Target = boost::target(E, U);

    for (const auto *SrcType : U[Src].types) {
      (*G)[Target].types.insert(SrcType);
    }
  }

  rev_graph_t *G;
};

CachedTypeGraph::vertex_t
CachedTypeGraph::addType(const llvm::StructType *NewType) {
  auto Name = NewType->getName().str();

  if (type_vertex_map.find(Name) == type_vertex_map.end()) {
    auto Vertex = boost::add_vertex(g);
    type_vertex_map[Name] = Vertex;
    g[Vertex].name = Name;
    g[Vertex].base_type = NewType;
    g[Vertex].types.insert(NewType);
  }

  return type_vertex_map[Name];
}

bool CachedTypeGraph::addLink(const llvm::StructType *From,
                              const llvm::StructType *To) {
  if (already_visited) {
    return false;
  }

  already_visited = true;

  auto FromVertex = addType(From);
  auto ToVertex = addType(To);

  auto ResultEdge = boost::add_edge(FromVertex, ToVertex, g);

  if (ResultEdge.second) {
    reverseTypePropagation(To);
  }

  already_visited = false;
  return ResultEdge.second;
}

bool CachedTypeGraph::addLinkWithoutReversePropagation(
    const llvm::StructType *From, const llvm::StructType *To) {
  if (already_visited) {
    return false;
  }

  already_visited = true;

  auto FromVertex = addType(From);
  auto ToVertex = addType(To);

  auto ResultEdge = boost::add_edge(FromVertex, ToVertex, g);

  already_visited = false;
  return ResultEdge.second;
}

void CachedTypeGraph::printAsDot(const std::string &Path) const {
  std::ofstream Ofs(Path);
  boost::write_graphviz(
      Ofs, g, boost::make_label_writer(boost::get(&VertexProperties::name, g)));
}

void CachedTypeGraph::aggregateTypes() {
  dfs_visitor Vis(&g);
  boost::depth_first_search(g, boost::visitor(Vis));
}

void CachedTypeGraph::reverseTypePropagation(
    const llvm::StructType *BaseStruct) {
  auto Name = BaseStruct->getName().str();

  std::vector<boost::default_color_type> ColorMap(boost::num_vertices(g));

  auto Reversed = boost::reverse_graph<CachedTypeGraph::graph_t,
                                       CachedTypeGraph::graph_t &>(g);
  reverse_type_propagation_dfs_visitor Vis(&Reversed);

  boost::depth_first_visit(Reversed, type_vertex_map[Name], Vis,
                           boost::make_iterator_property_map(
                               ColorMap.begin(),
                               boost::get(boost::vertex_index, Reversed),
                               ColorMap[0]));
}

std::set<const llvm::StructType *>
CachedTypeGraph::getTypes(const llvm::StructType *StructType) {
  auto StructTyVertex = addType(StructType);
  return g[StructTyVertex].types;
}

} // namespace psr
