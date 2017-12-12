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

#ifndef SRC_LIB_GRAPHEXTENSIONS_H_
#define SRC_LIB_GRAPHEXTENSIONS_H_

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/graph_utility.hpp>
#include <vector>

using namespace std;

// merges two graph by vertex-contraction
template <typename GraphTy, typename VertexTy, typename EdgeProperty,
          typename... Args>
void merge_graphs(GraphTy &g1, const GraphTy &g2,
                  vector<pair<VertexTy, VertexTy>> v_in_g1_u_in_g2,
                  Args &&... args) {
  typedef typename boost::property_map<GraphTy, boost::vertex_index_t>::type
      index_map_t;
  // for simple adjacency_list<> this type would be more efficient:
  typedef typename boost::iterator_property_map<
      typename std::vector<VertexTy>::iterator, index_map_t, VertexTy,
      VertexTy &>
      IsoMap;
  // for more generic graphs, one can try typedef std::map<vertex_t, vertex_t>
  // IsoMap;
  vector<VertexTy> orig2copy_data(boost::num_vertices(g2));
  IsoMap mapV = boost::make_iterator_property_map(orig2copy_data.begin(),
                                                  get(boost::vertex_index, g2));
  boost::copy_graph(g2, g1, boost::orig_to_copy(mapV)); // means g1 += g2
  for (auto &entry : v_in_g1_u_in_g2) {
    VertexTy u_in_g1 = mapV[entry.second];
    boost::add_edge(entry.first, u_in_g1, EdgeProperty(args...), g1);
  }
}

// merges two graph by vertex-contraction
template <typename GraphTy, typename VertexTy, typename EdgeProperty,
          typename Arg>
void merge_graphs(GraphTy &g1, const GraphTy &g2,
                  vector<tuple<VertexTy, VertexTy, Arg>> v_in_g1_u_in_g2_prop) {
  typedef typename boost::property_map<GraphTy, boost::vertex_index_t>::type
      index_map_t;
  // for simple adjacency_list<> this type would be more efficient:
  typedef typename boost::iterator_property_map<
      typename std::vector<VertexTy>::iterator, index_map_t, VertexTy,
      VertexTy &>
      IsoMap;
  // for more generic graphs, one can try typedef std::map<vertex_t, vertex_t>
  // IsoMap;
  vector<VertexTy> orig2copy_data(boost::num_vertices(g2));
  IsoMap mapV = boost::make_iterator_property_map(orig2copy_data.begin(),
                                                  get(boost::vertex_index, g2));
  boost::copy_graph(g2, g1, boost::orig_to_copy(mapV)); // means g1 += g2
  for (auto &entry : v_in_g1_u_in_g2_prop) {
    VertexTy u_in_g1 = mapV[get<1>(entry)];
    boost::add_edge(get<0>(entry), u_in_g1, EdgeProperty(get<2>(entry)), g1);
  }
}

template <typename GraphTy, typename VertexTy>
void copy_graph(GraphTy &g1, const GraphTy &g2) {
  typedef typename boost::property_map<GraphTy, boost::vertex_index_t>::type
      index_map_t;
  // for simple adjacency_list<> this type would be more efficient:
  typedef typename boost::iterator_property_map<
      typename std::vector<VertexTy>::iterator, index_map_t, VertexTy,
      VertexTy &>
      IsoMap;
  // for more generic graphs, one can try typedef std::map<vertex_t, vertex_t>
  // IsoMap;
  vector<VertexTy> orig2copy_data(boost::num_vertices(g2));
  IsoMap mapV = boost::make_iterator_property_map(orig2copy_data.begin(),
                                                  get(boost::vertex_index, g2));
  boost::copy_graph(g2, g1, boost::orig_to_copy(mapV)); // means g1 += g2
}

#endif /* SRC_LIB_GRAPHEXTENSIONS_HH_ */
