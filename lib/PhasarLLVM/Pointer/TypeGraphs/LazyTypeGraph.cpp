/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LazyLazyTypeGraph.cpp
 *
 *  Created on: 28.06.2018
 *      Author: nicolas bellec
 */

#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/graph_utility.hpp"
#include "boost/graph/graphviz.hpp"
#include "boost/property_map/dynamic_property_map.hpp"

#include "llvm/IR/DerivedTypes.h"

#include "phasar/PhasarLLVM/Pointer/TypeGraphs/LazyTypeGraph.h"

#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;

namespace psr {

struct LazyTypeGraph::dfs_visitor : public boost::default_dfs_visitor {
  dfs_visitor(std::set<const llvm::StructType *> &Result) : Result(Result) {}

  void finish_edge(edge_t E, graph_t const &U) {
    LazyTypeGraph::vertex_t Target = boost::target(E, U);

    Result.insert(U[Target].SType);
  }

  std::set<const llvm::StructType *> &Result;
};

LazyTypeGraph::vertex_t
LazyTypeGraph::addType(const llvm::StructType *NewType) {
  auto Name = NewType->getName().str();

  if (TypeToVertexMap.find(Name) == TypeToVertexMap.end()) {
    auto Vertex = boost::add_vertex(Graph);
    TypeToVertexMap[Name] = Vertex;
    Graph[Vertex].Name = Name;
    Graph[Vertex].SType = NewType;
  }

  return TypeToVertexMap[Name];
}

bool LazyTypeGraph::addLink(const llvm::StructType *From,
                            const llvm::StructType *To) {
  if (AlreadyVisited) {
    return false;
  }

  AlreadyVisited = true;

  auto FromVertex = addType(From);
  auto ToVertex = addType(To);

  auto ResultEdge = boost::add_edge(FromVertex, ToVertex, Graph);

  AlreadyVisited = false;
  return ResultEdge.second;
}

void LazyTypeGraph::printAsDot(const std::string &Path) const {
  std::ofstream Ofs(Path);
  boost::write_graphviz(Ofs, Graph,
                        boost::make_label_writer(boost::get(
                            &LazyTypeGraph::VertexProperties::Name, Graph)));
}

std::set<const llvm::StructType *>
LazyTypeGraph::getTypes(const llvm::StructType *StructType) {
  auto StructTyVertex = addType(StructType);

  std::vector<boost::default_color_type> ColorMap(boost::num_vertices(Graph));

  std::set<const llvm::StructType *> Results;
  Results.insert(StructType);

  dfs_visitor Vis(Results);

  boost::depth_first_visit(
      Graph, StructTyVertex, Vis,
      boost::make_iterator_property_map(ColorMap.begin(),
                                        boost::get(boost::vertex_index, Graph),
                                        ColorMap[0]));

  return Results;
}

} // namespace psr
