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

#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/None.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#if __cplusplus >= 202002L
#include <concepts>
#endif
#include <functional>
#include <string>
#include <type_traits>

namespace psr {

/// We aim to get rid of boost, so introduce a new GraphTraits class to replace
/// it.
/// This GraphTraits type should be specialized for each type that implements a
/// "graph". All the functionality should be reflected by the GraphTraits class.
/// Once moving to C++20, we have nice type-checking using concepts
template <typename Graph> struct GraphTraits;

#if __cplusplus >= 202002L

template <typename Edge>
concept is_graph_edge = requires(const Edge e1, Edge e2) {
  { e1 == e2 } -> std::convertible_to<bool>;
  { e1 != e2 } -> std::convertible_to<bool>;
  { e1 < e2 } -> std::convertible_to<bool>;
};

template <typename GraphTrait>
concept is_graph_trait = requires(typename GraphTrait::graph_type &graph,
                                  const typename GraphTrait::graph_type &cgraph,
                                  typename GraphTrait::value_type val,
                                  typename GraphTrait::vertex_t vtx,
                                  typename GraphTrait::edge_t edge) {
  typename GraphTrait::graph_type;
  typename GraphTrait::value_type;
  typename GraphTrait::vertex_t;
  typename GraphTrait::edge_t;
  requires is_graph_edge<typename GraphTrait::edge_t>;
  { GraphTrait::Invalid } -> std::convertible_to<typename GraphTrait::vertex_t>;
  {
    GraphTrait::addNode(graph, val)
    } -> std::convertible_to<typename GraphTrait::vertex_t>;
  {GraphTrait::addEdge(graph, vtx, edge)};
  {
    GraphTrait::outEdges(cgraph, vtx)
    } -> psr::is_iterable_over_v<typename GraphTrait::edge_t>;
  { GraphTrait::outDegree(cgraph, vtx) } -> std::convertible_to<size_t>;
  {GraphTrait::dedupOutEdges(graph, vtx)};
  {
    GraphTrait::nodes(cgraph)
    } -> psr::is_iterable_over_v<typename GraphTrait::value_type>;
  {
    GraphTrait::vertices(cgraph)
    } -> psr::is_iterable_over_v<typename GraphTrait::vertex_t>;
  {
    GraphTrait::node(cgraph, vtx)
    } -> std::convertible_to<typename GraphTrait::value_type>;
  { GraphTrait::size(cgraph) } -> std::convertible_to<size_t>;
  {GraphTrait::addRoot(graph, vtx)};
  {
    GraphTrait::roots(cgraph)
    } -> psr::is_iterable_over_v<typename GraphTrait::vertex_t>;
  { GraphTrait::pop(graph, vtx) } -> std::same_as<bool>;
  { GraphTrait::roots_size(cgraph) } -> std::convertible_to<size_t>;
  {
    GraphTrait::target(edge)
    } -> std::convertible_to<typename GraphTrait::vertex_t>;
  {
    GraphTrait::withEdgeTarget(edge, vtx)
    } -> std::convertible_to<typename GraphTrait::edge_t>;
  {GraphTrait::weight(edge)};
};

template <typename Graph>
concept is_graph = requires(Graph g) {
  typename GraphTraits<std::decay_t<Graph>>;
  requires is_graph_trait<GraphTraits<std::decay_t<Graph>>>;
};

template <typename GraphTrait>
concept is_reservable_graph_trait_v = is_graph_trait<GraphTrait> &&
    requires(typename GraphTrait::graph_type &g) {
  {GraphTrait::reserve(g, size_t(0))};
};

#else
namespace detail {
template <typename GraphTrait, typename = void>
// NOLINTNEXTLINE(readability-identifier-naming)
struct is_reservable_graph_trait : std::false_type {};
template <typename GraphTrait>
struct is_reservable_graph_trait<
    GraphTrait,
    std::void_t<decltype(GraphTrait::reserve(
        std::declval<typename GraphTrait::graph_type &>(), size_t()))>>
    : std::true_type {};

template <typename GraphTrait, typename = void>
// NOLINTNEXTLINE(readability-identifier-naming)
struct is_removable_graph_trait : std::false_type {};
template <typename GraphTrait>
struct is_removable_graph_trait<
    GraphTrait,
    std::void_t<typename GraphTrait::edge_iterator,
                typename GraphTrait::roots_iterator,
                decltype(GraphTrait::removeEdge(
                    std::declval<typename GraphTrait::graph_type &>(),
                    std::declval<typename GraphTrait::vertex_t>(),
                    std::declval<typename GraphTrait::edge_iterator>())),
                decltype(GraphTrait::removeRoot(
                    std::declval<typename GraphTrait::graph_type &>(),
                    std::declval<typename GraphTrait::roots_iterator>()))>>
    : std::true_type {};
} // namespace detail

template <typename GraphTrait>
// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr bool is_reservable_graph_trait_v =
    detail::is_reservable_graph_trait<GraphTrait>::value;

template <typename GraphTrait>
// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr bool is_removable_graph_trait_v =
    detail::is_removable_graph_trait<GraphTrait>::value;
#endif

template <typename GraphTy>
std::decay_t<GraphTy> reverseGraph(GraphTy &&G)
#if __cplusplus >= 202002L
    requires is_graph<GraphTy>
#endif
{
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

template <typename GraphTy, typename NodeTransform = DefaultNodeTransform>
void printGraph(const GraphTy &G, llvm::raw_ostream &OS,
                llvm::StringRef Name = "", NodeTransform NodeToString = {})
#if __cplusplus >= 202002L
    requires is_graph<GraphTy>
#endif
{
  using traits_t = GraphTraits<GraphTy>;

  OS << "digraph " << Name << " {\n";
  psr::scope_exit CloseBrace = [&OS] { OS << "}\n"; };

  auto Sz = traits_t::size(G);

  for (size_t I = 0; I < Sz; ++I) {
    OS << I;
    if constexpr (!std::is_same_v<llvm::NoneType,
                                  typename traits_t::value_type>) {
      OS << "[label=\"";
      OS.write_escaped(std::invoke(NodeToString, traits_t::node(G, I)));
      OS << "\"]";
    }
    OS << ";\n";
    for (const auto &Edge : traits_t::outEdges(G, I)) {
      OS << I << "->" << Edge << ";\n";
    }
  }
}

} // namespace psr

#endif // PHASAR_UTILS_GRAPHTRAITS_H
