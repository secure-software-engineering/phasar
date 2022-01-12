/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * GraphExtensions.hh
 *
 *  Created on: 06.04.2017
 *      Author: philipp
 */

#ifndef PHASAR_UTILS_GRAPHEXTENSIONS_H_
#define PHASAR_UTILS_GRAPHEXTENSIONS_H_

#include <utility> // std::pair
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/copy.hpp"
#include "boost/graph/graph_utility.hpp"

namespace psr {

template <typename GraphTy, typename VertexTy, typename EdgeProp>
void contractVertices(VertexTy Replacement, VertexTy Replace, GraphTy &Graph) {
  auto Be = boost::adjacent_vertices(Replace, Graph);
  auto BeIt = Be.first;
  while (BeIt != Be.second) {
    typename boost::graph_traits<GraphTy>::out_edge_iterator OutEdge, End;
    boost::tie(OutEdge, End) = boost::out_edges(Replace, Graph);
    while (OutEdge != End) {
      add_edge(Replacement, *BeIt, EdgeProp(Graph[*OutEdge]), Graph);
      remove_edge(*OutEdge, Graph);
      boost::tie(OutEdge, End) = boost::out_edges(Replace, Graph);
    }
    Be = boost::adjacent_vertices(Replace, Graph);
    BeIt = Be.first;
  }
  remove_vertex(Replace, Graph);
}

template <typename GraphTy, typename VertexTy, typename EdgeProp,
          typename... Args>
void mergeByStitching(GraphTy &Graph1, const GraphTy &Graph2,
                      std::vector<std::pair<VertexTy, VertexTy>> VsInG1UsInG2,
                      Args &&...Arguments) {
  using index_map_t =
      typename boost::property_map<GraphTy, boost::vertex_index_t>::type;
  // for simple adjacency_list<> this type would be more efficient:
  using IsoMap = typename boost::iterator_property_map<
      typename std::vector<VertexTy>::iterator, index_map_t, VertexTy,
      VertexTy &>;
  // for more generic graphs, one can try  //typedef std::map<vertex_t,
  // vertex_t> IsoMap;
  std::vector<VertexTy> OrigToCopyData(num_vertices(Graph2));
  IsoMap MapV = make_iterator_property_map(OrigToCopyData.begin(),
                                           get(boost::vertex_index, Graph2));
  boost::copy_graph(Graph2, Graph1,
                    boost::orig_to_copy(MapV)); // means g1 += g2
  for (auto VInG1UInG2 : VsInG1UsInG2) {
    VertexTy UInG1 = MapV[VInG1UInG2.second];
    boost::add_edge(VInG1UInG2.first, UInG1, EdgeProperties(Arguments...),
                    Graph1);
  }
}

// merges two graph by vertex-contraction
template <typename GraphTy, typename VertexTy, typename EdgeProperty,
          typename... Args>
void mergeGraphs(GraphTy &Graph1, const GraphTy &Graph2,
                 std::vector<std::pair<VertexTy, VertexTy>> VInG1UInG2,
                 Args &&...Arguments) {
  using index_map_t =
      typename boost::property_map<GraphTy, boost::vertex_index_t>::type;
  // for simple adjacency_list<> this type would be more efficient:
  using IsoMap = typename boost::iterator_property_map<
      typename std::vector<VertexTy>::iterator, index_map_t, VertexTy,
      VertexTy &>;
  // for more generic graphs, one can try typedef std::map<vertex_t, vertex_t>
  // IsoMap;
  std::vector<VertexTy> OrigToCopyData(boost::num_vertices(Graph2));
  IsoMap MapV = boost::make_iterator_property_map(
      OrigToCopyData.begin(), get(boost::vertex_index, Graph2));
  boost::copy_graph(Graph2, Graph1,
                    boost::orig_to_copy(MapV)); // means g1 += g2
  for (auto &Entry : VInG1UInG2) {
    VertexTy UInG1 = MapV[Entry.second];
    boost::add_edge(Entry.first, UInG1, EdgeProperty(Arguments...), Graph1);
  }
}

// merges two graph by vertex-contraction
template <typename GraphTy, typename VertexTy, typename EdgeProperty,
          typename Arg>
void mergeGraphs(
    GraphTy &Graph1, const GraphTy &Graph2,
    std::vector<std::tuple<VertexTy, VertexTy, Arg>> VInG1UInG2Prop) {
  using index_map_t =
      typename boost::property_map<GraphTy, boost::vertex_index_t>::type;
  // for simple adjacency_list<> this type would be more efficient:
  using IsoMap = typename boost::iterator_property_map<
      typename std::vector<VertexTy>::iterator, index_map_t, VertexTy,
      VertexTy &>;
  // for more generic graphs, one can try typedef std::map<vertex_t, vertex_t>
  // IsoMap;
  std::vector<VertexTy> OrigToCopyData(boost::num_vertices(Graph2));
  IsoMap MapV = boost::make_iterator_property_map(
      OrigToCopyData.begin(), get(boost::vertex_index, Graph2));
  boost::copy_graph(Graph2, Graph1,
                    boost::orig_to_copy(MapV)); // means g1 += g2
  for (auto &Entry : VInG1UInG2Prop) {
    VertexTy UInG1 = MapV[get<1>(Entry)];
    boost::add_edge(get<0>(Entry), UInG1, EdgeProperty(get<2>(Entry)), Graph1);
  }
}

template <typename GraphTy, typename VertexTy>
void copyGraph(GraphTy &Graph1, const GraphTy &Graph2) {
  using index_map_t =
      typename boost::property_map<GraphTy, boost::vertex_index_t>::type;
  // for simple adjacency_list<> this type would be more efficient:
  using IsoMap = typename boost::iterator_property_map<
      typename std::vector<VertexTy>::iterator, index_map_t, VertexTy,
      VertexTy &>;
  // for more generic graphs, one can try typedef std::map<vertex_t, vertex_t>
  // IsoMap;
  std::vector<VertexTy> OrigToCopyData(boost::num_vertices(Graph2));
  IsoMap MapV = boost::make_iterator_property_map(
      OrigToCopyData.begin(), get(boost::vertex_index, Graph2));
  boost::copy_graph(Graph2, Graph1,
                    boost::orig_to_copy(MapV)); // means g1 += g2
}

} // namespace psr

#endif
