/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#ifndef PHASAR_UTILS_ADJACENCYLIST_H
#define PHASAR_UTILS_ADJACENCYLIST_H

#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/RepeatIterator.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/SmallVector.h"

#include <iterator>
#include <limits>
#include <type_traits>

namespace psr {

template <typename T, typename EdgeTy = unsigned> struct AdjacencyList {
  llvm::SmallVector<T, 0> Nodes{};
  llvm::SmallVector<llvm::SmallVector<EdgeTy, 2>, 0> Adj{};
  llvm::SmallVector<unsigned, 1> Roots{};
};

template <typename EdgeTy> struct AdjacencyList<llvm::NoneType, EdgeTy> {
  llvm::SmallVector<llvm::SmallVector<EdgeTy, 2>, 0> Adj{};
  llvm::SmallVector<unsigned, 1> Roots{};
};

template <typename T, typename EdgeTy>
struct GraphTraits<AdjacencyList<T, EdgeTy>> {
  using graph_type = AdjacencyList<T, EdgeTy>;
  using value_type = T;
  using vertex_t = unsigned;
  using edge_t = EdgeTy;
  using edge_iterator = typename llvm::ArrayRef<edge_t>::const_iterator;
  using roots_iterator = typename llvm::ArrayRef<vertex_t>::const_iterator;

  static inline constexpr auto Invalid = std::numeric_limits<vertex_t>::max();

  /// Adds a new node to the graph G with node-tag Val
  ///
  /// \returns The vertex-descriptor for the newly created node
  template <typename TT,
            typename = std::enable_if_t<!std::is_same_v<TT, llvm::NoneType>>>
  static vertex_t addNode(graph_type &G, TT &&Val) {
    assert(G.Adj.size() == G.Nodes.size());

    auto Ret = G.Nodes.size();
    G.Nodes.push_back(std::forward<TT>(Val));
    G.Adj.emplace_back();
    return Ret;
  }

  /// Adds a new node to the graph G without node-tag
  ///
  /// \returns The vertex-descriptor for the newly created node
  template <typename TT = value_type,
            typename = std::enable_if_t<std::is_same_v<TT, llvm::NoneType>>>
  static vertex_t addNode(graph_type &G, llvm::NoneType /*Val*/ = llvm::None) {
    auto Ret = G.Adj.size();
    G.Adj.emplace_back();
    return Ret;
  }

  /// Makes the node Vtx as root in the graph G. A node should not be registered
  /// as root multiple times
  static void addRoot(graph_type &G, vertex_t Vtx) {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(Vtx < G.Nodes.size());
      assert(G.Adj.size() == G.Nodes.size());
    }
    G.Roots.push_back(Vtx);
  }

  /// Gets a range of all root nodes of graph G
  static llvm::ArrayRef<vertex_t> roots(const graph_type &G) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Roots;
  }

  /// Adds a new edge from node From to node To in graph G. From and T should be
  /// nodes inside G. Multi-edges are supported, i.e. edges are not deduplicated
  /// automatically; to manualy deduplicate the edges of one source-node, call
  /// dedupOutEdges()
  static void addEdge(graph_type &G, vertex_t From, edge_t To) {
    assert(From < G.Adj.size());
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    G.Adj[From].push_back(std::move(To));
  }

  /// Gets a range of all edges outgoing from node Vtx in graph G
  static llvm::ArrayRef<edge_t> outEdges(const graph_type &G,
                                         vertex_t Vtx) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(Vtx < G.Nodes.size());
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Adj[Vtx];
  }

  /// Gets the number of edges outgoing from node Vtx in graph G
  static size_t outDegree(const graph_type &G, vertex_t Vtx) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(Vtx < G.Nodes.size());
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Adj[Vtx].size();
  }

  /// Deduplicates the edges outgoing from node Vtx in graph G. Deduplication is
  /// based on operator< and operator== of the edge_t type
  static void dedupOutEdges(graph_type &G, vertex_t Vtx) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(Vtx < G.Nodes.size());
      assert(G.Adj.size() == G.Nodes.size());
    }
    auto &OutEdges = G.Adj[Vtx];
    std::sort(OutEdges.begin(), OutEdges.end());
    OutEdges.erase(std::unique(OutEdges.begin(), OutEdges.end()),
                   OutEdges.end());
  }

  /// Gets a const range of all nodes in graph G
  template <typename TT = value_type,
            typename = std::enable_if_t<!std::is_same_v<TT, llvm::NoneType>>>
  static llvm::ArrayRef<value_type> nodes(const graph_type &G) noexcept {
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes;
  }
  /// Gets a mutable range of all nodes in graph G
  template <typename TT = value_type>
  static std::enable_if_t<!std::is_same_v<TT, llvm::NoneType>,
                          llvm::MutableArrayRef<value_type>>
  nodes(graph_type &G) noexcept {
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes;
  }
  /// Gets a range of all nodes in graph G
  template <typename TT = value_type,
            typename = std::enable_if_t<std::is_same_v<TT, llvm::NoneType>>>
  static std::enable_if_t<std::is_same_v<TT, llvm::NoneType>,
                          RepeatRangeType<value_type>>
  nodes(const graph_type &G) noexcept {
    return repeat(llvm::None, G.Adj.size());
  }

  /// Gets a range of vertex-descriptors for all nodes in graph G
  static auto vertices(const graph_type &G) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return psr::iota(size_t(0), G.Adj.size());
  }

  /// Gets the node-tag for node Vtx in graph G. Vtx must be part of G
  template <typename TT = value_type,
            typename = std::enable_if_t<!std::is_same_v<TT, llvm::NoneType>>>
  static const value_type &node(const graph_type &G, vertex_t Vtx) noexcept {
    assert(Vtx < G.Nodes.size());
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes[Vtx];
  }
  /// Gets the node-tag for node Vtx in graph G. Vtx must be part of G
  template <typename TT = value_type,
            typename = std::enable_if_t<!std::is_same_v<TT, llvm::NoneType>>>
  static value_type &node(graph_type &G, vertex_t Vtx) noexcept {
    assert(Vtx < G.Nodes.size());
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes[Vtx];
  }

  /// Gets the node-tag for node Vtx in graph G. Vtx must be part of G
  template <typename TT = value_type,
            typename = std::enable_if_t<std::is_same_v<TT, llvm::NoneType>>>
  static llvm::NoneType node([[maybe_unused]] const graph_type &G,
                             [[maybe_unused]] vertex_t Vtx) noexcept {
    assert(Vtx < G.Adj.size());
    return llvm::None;
  }

  /// Gets the number of nodes in graph G
  static size_t size(const graph_type &G) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Adj.size();
  }

  /// Gets the number of nodes in graph G that are marked as root
  static size_t roots_size(const graph_type &G) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Roots.size();
  }

  /// Pre-allocates space to hold up to Capacity nodes
  static void reserve(graph_type &G, size_t Capacity) {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
      G.Nodes.reserve(Capacity);
    }

    G.Adj.reserve(Capacity);
  }

  /// Tries to remove the last inserted node Vtx fro graph G. Fails, if there
  /// was another not-popped node inserted in between.
  ///
  /// \returns True, iff the removal was successful
  static bool pop(graph_type &G, vertex_t Vtx) {
    if (Vtx == G.Adj.size() - 1) {
      G.Adj.pop_back();
      if constexpr (!std::is_same_v<llvm::NoneType, value_type>) {
        G.Nodes.pop_back();
      }
      return true;
    }
    return false;
  }

  /// Gets the vertex-descriptor of the target-node of the given Edge
  template <typename E = edge_t>
  static std::enable_if_t<std::is_same_v<E, vertex_t>, vertex_t>
  target(edge_t Edge) noexcept {
    return Edge;
  }

  /// Copies the given edge, but replaces the target node by Tar; i.e. the
  /// weight of the returned edge and the passed edge is same, but the target
  /// nodes may differ.
  template <typename E = edge_t>
  static std::enable_if_t<std::is_same_v<E, vertex_t>, edge_t>
  withEdgeTarget(edge_t /*edge*/, vertex_t Tar) noexcept {
    return Tar;
  }

  /// Gets the weight associated with the given edge
  static llvm::NoneType weight(edge_t /*unused*/) noexcept {
    return llvm::None;
  }

  static edge_iterator removeEdge(graph_type &G, vertex_t Vtx,
                                  edge_iterator It) noexcept {
    assert(Vtx < G.Nodes.size());
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    assert(G.Adj[Vtx].begin() <= It && It < G.Adj[Vtx].end());
    auto Idx = std::distance(std::cbegin(G.Adj[Vtx]), It);

    std::swap(G.Adj[Vtx][Idx], G.Adj[Vtx].back());
    G.Adj[Vtx].pop_back();
    return It;
  }

  static roots_iterator removeRoot(graph_type &G, roots_iterator It) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    assert(G.Roots.begin() <= It && It < G.Roots.end());

    auto Idx = std::distance(std::cbegin(G.Roots), It);
    std::swap(G.Roots[Idx], G.Roots.back());
    G.Roots.pop_back();
    return It;
  }

#if __cplusplus >= 202002L
  static_assert(is_graph<AdjacencyList<T>>);
  static_assert(is_reservable_graph_trait<GraphTraits<AdjacencyList<T>>>);
#endif
};

} // namespace psr

#endif // PHASAR_UTILS_ADJACENCYLIST_H
