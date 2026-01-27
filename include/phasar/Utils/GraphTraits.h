/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#ifndef PHASAR_UTILS_GRAPHTRAITS_H
#define PHASAR_UTILS_GRAPHTRAITS_H

#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <concepts>
#include <functional>
#include <string>
#include <type_traits>

namespace psr {

/// We removed the dependency to boost, so introduce a new GraphTraits class to
/// replace it. This GraphTraits type should be specialized for each type that
/// implements a "graph". All the functionality should be reflected by the
/// GraphTraits class. Once moving to C++20, we have nice type-checking using
/// concepts
template <typename Graph> struct GraphTraits;

template <typename Edge>
concept is_graph_edge = requires(const Edge E1, Edge E2) {
  { E1 == E2 } -> std::convertible_to<bool>;
  { E1 != E2 } -> std::convertible_to<bool>;
};

template <typename GraphTrait>
concept is_const_graph_trait =
    requires(const typename GraphTrait::graph_type &CGraph,
             typename GraphTrait::value_type Val,
             typename GraphTrait::vertex_t Vtx,
             typename GraphTrait::edge_t Edge) {
      typename GraphTrait::graph_type;
      typename GraphTrait::value_type;
      typename GraphTrait::vertex_t;
      typename GraphTrait::edge_t;
      requires is_graph_edge<typename GraphTrait::edge_t>;

      {
        GraphTrait::Invalid
      } -> std::convertible_to<typename GraphTrait::vertex_t>;

      {
        GraphTrait::outEdges(CGraph, Vtx)
      } -> psr::is_iterable_over_v<typename GraphTrait::edge_t>;
      { GraphTrait::outDegree(CGraph, Vtx) } -> std::convertible_to<size_t>;
      {
        GraphTrait::nodes(CGraph)
      } -> psr::is_iterable_over_v<typename GraphTrait::value_type>;
      {
        GraphTrait::roots(CGraph)
      } -> psr::is_iterable_over_v<typename GraphTrait::vertex_t>;
      {
        GraphTrait::vertices(CGraph)
      } -> psr::is_iterable_over_v<typename GraphTrait::vertex_t>;
      {
        GraphTrait::node(CGraph, Vtx)
      } -> std::convertible_to<typename GraphTrait::value_type>;
      { GraphTrait::size(CGraph) } -> std::convertible_to<size_t>;
      { GraphTrait::roots_size(CGraph) } -> std::convertible_to<size_t>;
      {
        GraphTrait::target(Edge)
      } -> std::convertible_to<typename GraphTrait::vertex_t>;
      {
        GraphTrait::withEdgeTarget(Edge, Vtx)
      } -> std::convertible_to<typename GraphTrait::edge_t>;
    };

template <typename GraphTrait>
concept is_graph_trait =
    is_const_graph_trait<GraphTrait> &&
    requires(typename GraphTrait::graph_type &Graph,
             typename GraphTrait::value_type Val,
             typename GraphTrait::vertex_t Vtx,
             typename GraphTrait::edge_t Edge) {
      {
        GraphTrait::addNode(Graph, Val)
      } -> std::convertible_to<typename GraphTrait::vertex_t>;
      { GraphTrait::addEdge(Graph, Vtx, Edge) };
      { GraphTrait::dedupOutEdges(Graph, Vtx) };
      { GraphTrait::addRoot(Graph, Vtx) };
      { GraphTrait::pop(Graph, Vtx) } -> std::same_as<bool>;
    };

template <typename GraphTrait>
concept is_weighted_const_graph_trait =
    is_const_graph_trait<GraphTrait> &&
    requires(const typename GraphTrait::edge_t &Edge) {
      typename GraphTrait::weight_t;
      {
        GraphTrait::weight(Edge)
      } -> std::convertible_to<typename GraphTrait::weight_t>;
    };
template <typename GraphTrait>
concept is_weighted_graph_trait =
    is_graph_trait<GraphTrait> && is_weighted_const_graph_trait<GraphTrait>;

template <typename Graph>
concept is_const_graph = requires(Graph G) {
  typename GraphTraits<std::decay_t<Graph>>;
  requires is_const_graph_trait<GraphTraits<std::decay_t<Graph>>>;
};

template <typename Graph>
concept is_graph = requires(Graph G) {
  typename GraphTraits<std::decay_t<Graph>>;
  requires is_graph_trait<GraphTraits<std::decay_t<Graph>>>;
};

template <typename Graph>
concept is_weighted_const_graph = requires(Graph G) {
  typename GraphTraits<std::decay_t<Graph>>;
  requires is_weighted_const_graph_trait<GraphTraits<std::decay_t<Graph>>>;
};

template <typename Graph>
concept is_weighted_graph = requires(Graph G) {
  typename GraphTraits<std::decay_t<Graph>>;
  requires is_weighted_graph_trait<GraphTraits<std::decay_t<Graph>>>;
};

template <typename GraphTrait>
concept is_reservable_graph_trait_v =
    is_graph_trait<GraphTrait> && requires(typename GraphTrait::graph_type &G) {
      { GraphTrait::reserve(G, size_t(0)) };
    };

template <typename GraphTrait>
concept is_removable_graph_trait_v =
    is_graph_trait<GraphTrait> &&
    requires(typename GraphTrait::graph_type &G,
             typename GraphTrait::vertex_t Vtx,
             typename GraphTrait::edge_iterator EdgeIt,
             typename GraphTrait::roots_iterator RootIt) {
      typename GraphTrait::edge_iterator;
      typename GraphTrait::roots_iterator;
      { GraphTrait::removeEdge(G, Vtx, EdgeIt) };
      { GraphTrait::removeRoot(G, RootIt) };
    };

template <is_graph GraphTy> std::decay_t<GraphTy> reverseGraph(GraphTy &&G) {
  std::decay_t<GraphTy> Ret;
  using traits_t = GraphTraits<std::decay_t<GraphTy>>;
  if constexpr (is_reservable_graph_trait_v<traits_t>) {
    traits_t::reserve(Ret, traits_t::size(G));
  }

  for (auto &Nod : traits_t::nodes(G)) {
    traits_t::addNode(Ret, forward_like<GraphTy>(Nod));
  }
  for (auto I : traits_t::vertices(G)) {
    for (auto Child : traits_t::outEdges(G, I)) {
      traits_t::addEdge(Ret, traits_t::target(Child),
                        traits_t::withEdgeTarget(Child, I));
    }
    if (traits_t::outDegree(G, I) == 0) {
      traits_t::addRoot(Ret, I);
    }
  }
  return Ret;
}

struct DefaultNodeTransform {
  template <typename N> std::string operator()(const N &Nod) const {
    std::string Buf;
    llvm::raw_string_ostream ROS(Buf);
    ROS << Nod;
    ROS.flush();
    return Buf;
  }
};

/// \brief Prints the given graph G as dot.
///
/// \param G The graph to print
/// \param OS The output-stream, where to print into
/// \param Name The name of the graph
/// \param NodeToString If the graph has node-labels, convert a node-label to
/// string
template <is_const_graph GraphTy, typename NodeTransform = DefaultNodeTransform>
void printGraph(const GraphTy &G, llvm::raw_ostream &OS,
                llvm::StringRef Name = "", NodeTransform NodeToString = {}) {
  using traits_t = GraphTraits<GraphTy>;

  OS << "digraph \"";
  OS.write_escaped(Name) << "\" {\n";
  psr::scope_exit CloseBrace = [&OS] { OS << "}\n"; };

  for (auto Vtx : traits_t::vertices(G)) {
    OS << size_t(Vtx);
    if constexpr (!std::is_empty_v<typename traits_t::value_type>) {
      OS << "[label=\"";
      OS.write_escaped(std::invoke(NodeToString, traits_t::node(G, Vtx)));
      OS << "\"]";
    }
    OS << ";\n";
    for (const auto &Edge : traits_t::outEdges(G, Vtx)) {
      OS << size_t(Vtx) << "->";
      if constexpr (is_llvm_printable_v<decltype(Edge)>) {
        // to print the edge-weight as well, if possible
        OS << Edge;
      } else {
        OS << size_t(traits_t::target(Edge));
      }
      OS << ";\n";
    }
  }
}

} // namespace psr

#endif // PHASAR_UTILS_GRAPHTRAITS_H
