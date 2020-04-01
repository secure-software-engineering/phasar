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
  dfs_visitor(graph_t *_g) : g(_g) {}

  void finish_edge(edge_t e, graph_t const &u) {
    CachedTypeGraph::vertex_t src = boost::source(e, u);
    CachedTypeGraph::vertex_t target = boost::target(e, u);

    for (auto target_type : u[target].types) {
      (*g)[src].types.insert(target_type);
    }
  }

  graph_t *g;
};

struct CachedTypeGraph::reverse_type_propagation_dfs_visitor
    : public boost::default_dfs_visitor {
  reverse_type_propagation_dfs_visitor(rev_graph_t *_g) : g(_g) {}

  void examine_edge(rev_edge_t e, rev_graph_t const &u) {
    auto src = boost::source(e, u);
    auto target = boost::target(e, u);

    for (auto src_type : u[src].types)
      (*g)[target].types.insert(src_type);
  }

  rev_graph_t *g;
};

CachedTypeGraph::vertex_t
CachedTypeGraph::addType(const llvm::StructType *new_type) {
  auto name = new_type->getName().str();

  if (type_vertex_map.find(name) == type_vertex_map.end()) {
    auto vertex = boost::add_vertex(g);
    type_vertex_map[name] = vertex;
    g[vertex].name = name;
    g[vertex].base_type = new_type;
    g[vertex].types.insert(new_type);
  }

  return type_vertex_map[name];
}

bool CachedTypeGraph::addLink(const llvm::StructType *from,
                              const llvm::StructType *to) {
  if (already_visited)
    return false;

  already_visited = true;

  auto from_vertex = addType(from);
  auto to_vertex = addType(to);

  auto result_edge = boost::add_edge(from_vertex, to_vertex, g);

  if (result_edge.second) {
    reverseTypePropagation(to);
  }

  already_visited = false;
  return result_edge.second;
}

bool CachedTypeGraph::addLinkWithoutReversePropagation(
    const llvm::StructType *from, const llvm::StructType *to) {
  if (already_visited)
    return false;

  already_visited = true;

  auto from_vertex = addType(from);
  auto to_vertex = addType(to);

  auto result_edge = boost::add_edge(from_vertex, to_vertex, g);

  already_visited = false;
  return result_edge.second;
}

void CachedTypeGraph::printAsDot(const std::string &path) const {
  std::ofstream ofs(path);
  boost::write_graphviz(
      ofs, g, boost::make_label_writer(boost::get(&VertexProperties::name, g)));
}

void CachedTypeGraph::aggregateTypes() {
  dfs_visitor vis(&g);
  boost::depth_first_search(g, boost::visitor(vis));
}

void CachedTypeGraph::reverseTypePropagation(
    const llvm::StructType *base_struct) {
  auto name = base_struct->getName().str();

  std::vector<boost::default_color_type> color_map(boost::num_vertices(g));

  auto reversed = boost::reverse_graph<CachedTypeGraph::graph_t,
                                       CachedTypeGraph::graph_t &>(g);
  reverse_type_propagation_dfs_visitor vis(&reversed);

  boost::depth_first_visit(reversed, type_vertex_map[name], vis,
                           boost::make_iterator_property_map(
                               color_map.begin(),
                               boost::get(boost::vertex_index, reversed),
                               color_map[0]));
}

std::set<const llvm::StructType *>
CachedTypeGraph::getTypes(const llvm::StructType *struct_type) {
  auto struct_ty_vertex = addType(struct_type);
  return g[struct_ty_vertex].types;
}

} // namespace psr
