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

    for (const auto *TargetType : U[Target].Types) {
      (*G)[Src].Types.insert(TargetType);
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

    for (const auto *SrcType : U[Src].Types) {
      (*G)[Target].Types.insert(SrcType);
    }
  }

  rev_graph_t *G;
};

CachedTypeGraph::vertex_t
CachedTypeGraph::addType(const llvm::StructType *NewType) {
  std::string Name;
  if (!NewType->isLiteral()) {
    Name = NewType->getName().str();
  } else {
    std::stringstream StrS;
    StrS << "literal_" << NewType;
    Name = StrS.str();
  }

  if (TypeVertexMap.find(Name) == TypeVertexMap.end()) {
    auto Vertex = boost::add_vertex(G);
    TypeVertexMap[Name] = Vertex;
    G[Vertex].Name = Name;
    G[Vertex].BaseType = NewType;
    G[Vertex].Types.insert(NewType);
  }

  return TypeVertexMap[Name];
}

bool CachedTypeGraph::addLink(const llvm::StructType *From,
                              const llvm::StructType *To) {
  if (AlreadyVisited) {
    return false;
  }

  AlreadyVisited = true;

  auto FromVertex = addType(From);
  auto ToVertex = addType(To);

  auto ResultEdge = boost::add_edge(FromVertex, ToVertex, G);

  if (ResultEdge.second) {
    reverseTypePropagation(To);
  }

  AlreadyVisited = false;
  return ResultEdge.second;
}

bool CachedTypeGraph::addLinkWithoutReversePropagation(
    const llvm::StructType *From, const llvm::StructType *To) {
  if (AlreadyVisited) {
    return false;
  }

  AlreadyVisited = true;

  auto FromVertex = addType(From);
  auto ToVertex = addType(To);

  auto ResultEdge = boost::add_edge(FromVertex, ToVertex, G);

  AlreadyVisited = false;
  return ResultEdge.second;
}

void CachedTypeGraph::printAsDot(const std::string &Path) const {
  std::ofstream Ofs(Path);
  boost::write_graphviz(
      Ofs, G, boost::make_label_writer(boost::get(&VertexProperties::Name, G)));
}

void CachedTypeGraph::aggregateTypes() {
  dfs_visitor Vis(&G);
  boost::depth_first_search(G, boost::visitor(Vis));
}

void CachedTypeGraph::reverseTypePropagation(
    const llvm::StructType *BaseStruct) {
  auto Name = BaseStruct->getName().str();

  std::vector<boost::default_color_type> ColorMap(boost::num_vertices(G));

  auto Reversed = boost::reverse_graph<CachedTypeGraph::graph_t,
                                       CachedTypeGraph::graph_t &>(G);
  reverse_type_propagation_dfs_visitor Vis(&Reversed);

  boost::depth_first_visit(Reversed, TypeVertexMap[Name], Vis,
                           boost::make_iterator_property_map(
                               ColorMap.begin(),
                               boost::get(boost::vertex_index, Reversed),
                               ColorMap[0]));
}

std::set<const llvm::StructType *>
CachedTypeGraph::getTypes(const llvm::StructType *StructType) {
  auto StructTyVertex = addType(StructType);
  return G[StructTyVertex].Types;
}

} // namespace psr
