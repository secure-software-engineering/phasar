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

#include "phasar/Utils/EmptyBaseOptimizationUtils.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/RepeatIterator.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <type_traits>

namespace psr {

template <typename T, typename VtxId = uint32_t, typename EdgeTy = VtxId>
struct AdjacencyList {
  TypedVector<VtxId, T, 0> Nodes{};
  TypedVector<VtxId, llvm::SmallVector<EdgeTy, 2>, 0> Adj{};
  llvm::SmallVector<VtxId, 1> Roots{};
};

template <typename VtxId, typename EdgeTy>
struct AdjacencyList<EmptyType, VtxId, EdgeTy> {
  TypedVector<VtxId, llvm::SmallVector<EdgeTy, 2>, 0> Adj{};
  llvm::SmallVector<VtxId, 1> Roots{};
};

/// A simple graph implementation based on an adjacency list
template <typename T, typename VtxId, typename EdgeTy>
struct GraphTraits<AdjacencyList<T, VtxId, EdgeTy>> {
  using graph_type = AdjacencyList<T, VtxId, EdgeTy>;
  using value_type = T;
  using vertex_t = VtxId;
  using edge_t = EdgeTy;
  using edge_iterator = typename llvm::ArrayRef<edge_t>::const_iterator;
  using roots_iterator = typename llvm::ArrayRef<vertex_t>::const_iterator;

  /// A vertex that is not inserted into any graph. Can be used to communicate
  /// failure of certain operations
  static inline constexpr auto Invalid = std::numeric_limits<vertex_t>::max();

  /// Adds a new node to the graph G with node-tag Val
  ///
  /// \returns The vertex-descriptor for the newly created node
  template <typename TT = value_type,
            typename = std::enable_if_t<!std::is_empty_v<TT>>>
  static constexpr vertex_t addNode(graph_type &G, TT &&Val) {
    assert(G.Adj.size() == G.Nodes.size());

    auto Ret = vertex_t(G.Nodes.size());
    G.Nodes.push_back(std::forward<TT>(Val));
    G.Adj.emplace_back();
    return Ret;
  }

  /// Adds a new node to the graph G without node-tag
  ///
  /// \returns The vertex-descriptor for the newly created node
  template <typename TT = value_type,
            typename = std::enable_if_t<std::is_empty_v<TT>>>
  static constexpr vertex_t addNode(graph_type &G, value_type /*Val*/ = {}) {
    auto Ret = vertex_t(G.Adj.size());
    G.Adj.emplace_back();
    return Ret;
  }

  /// Makes the node Vtx as root in the graph G. A node should not be registered
  /// as root multiple times
  static constexpr void addRoot(graph_type &G, vertex_t Vtx) {
    assert(G.Adj.inbounds(Vtx));
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    G.Roots.push_back(Vtx);
  }

  /// Gets a range of all root nodes of graph G
  static constexpr llvm::ArrayRef<vertex_t>
  roots(const graph_type &G) noexcept {
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Roots;
  }

  /// Adds a new edge from node From to node To in graph G. From and To should
  /// be nodes inside G. Multi-edges are supported, i.e. edges are not
  /// deduplicated automatically; to manually deduplicate the edges of one
  /// source-node, call dedupOutEdges()
  static constexpr void addEdge(graph_type &G, vertex_t From, edge_t To) {
    assert(G.Adj.inbounds(From));
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    G.Adj[From].push_back(std::move(To));
  }

  /// Gets a range of all edges outgoing from node Vtx in graph G
  static constexpr llvm::ArrayRef<edge_t> outEdges(const graph_type &G,
                                                   vertex_t Vtx) noexcept {
    assert(G.Adj.inbounds(Vtx));
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Adj[Vtx];
  }

  /// Gets the number of edges outgoing from node Vtx in graph G
  static constexpr size_t outDegree(const graph_type &G,
                                    vertex_t Vtx) noexcept {
    assert(G.Adj.inbounds(Vtx));
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Adj[Vtx].size();
  }

  /// Deduplicates the edges outgoing from node Vtx in graph G. Deduplication is
  /// based on operator== of the edge_t type, and operator< if available.
  static constexpr void dedupOutEdges(graph_type &G, vertex_t Vtx) noexcept {
    assert(G.Adj.inbounds(Vtx));
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    auto &OutEdges = G.Adj[Vtx];

    if constexpr (IsLessComparable<edge_t>) {
      std::sort(OutEdges.begin(), OutEdges.end());
      OutEdges.erase(std::unique(OutEdges.begin(), OutEdges.end()),
                     OutEdges.end());
    } else {
      auto End = OutEdges.end();
      for (auto It = OutEdges.begin(); It < End; ++It) {
        End = std::remove(std::next(It), End, *It);
      }
      OutEdges.erase(End, OutEdges.end());
    }
  }

  /// Gets a const range of all nodes in graph G
  template <typename TT = value_type,
            typename = std::enable_if_t<!std::is_empty_v<TT>>>
  static constexpr const auto &nodes(const graph_type &G) noexcept {
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes;
  }
  /// Gets a mutable range of all nodes in graph G
  template <typename TT = value_type,
            typename = std::enable_if_t<!std::is_empty_v<TT>>>
  static constexpr auto &nodes(graph_type &G) noexcept {
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes;
  }
  /// Gets a range of all nodes in graph G
  template <typename TT = value_type,
            typename = std::enable_if_t<std::is_empty_v<TT>>>
  static constexpr RepeatRangeType<value_type>
  nodes(const graph_type &G) noexcept {
    return repeat(value_type{}, G.Adj.size());
  }

  /// Gets a range of vertex-descriptors for all nodes in graph G
  static constexpr auto vertices(const graph_type &G) noexcept {
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return psr::iota<vertex_t>(G.Adj.size());
  }

  /// Gets the node-tag for node Vtx in graph G. Vtx must be part of G
  template <typename TT = value_type,
            typename = std::enable_if_t<!std::is_empty_v<TT>>>
  static constexpr const value_type &node(const graph_type &G,
                                          vertex_t Vtx) noexcept {
    assert(G.Adj.inbounds(Vtx));
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes[Vtx];
  }
  /// Gets the node-tag for node Vtx in graph G. Vtx must be part of G
  template <typename TT = value_type,
            typename = std::enable_if_t<!std::is_empty_v<TT>>>
  static constexpr value_type &node(graph_type &G, vertex_t Vtx) noexcept {
    assert(G.Adj.inbounds(Vtx));
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes[Vtx];
  }

  /// Gets the node-tag for node Vtx in graph G. Vtx must be part of G
  template <typename TT = value_type,
            typename = std::enable_if_t<std::is_empty_v<TT>>>
  static constexpr value_type node([[maybe_unused]] const graph_type &G,
                                   [[maybe_unused]] vertex_t Vtx) noexcept {
    assert(G.Adj.inbounds(Vtx));
    return {};
  }

  /// Gets the number of nodes in graph G
  static constexpr size_t size(const graph_type &G) noexcept {
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Adj.size();
  }

  /// Gets the number of nodes in graph G that are marked as root
  static constexpr size_t roots_size(const graph_type &G) noexcept { // NOLINT
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Roots.size();
  }

  /// Pre-allocates space to hold up to Capacity nodes
  static constexpr void reserve(graph_type &G, size_t Capacity) {
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
      G.Nodes.reserve(Capacity);
    }

    G.Adj.reserve(Capacity);
  }

  /// Tries to remove the last inserted node Vtx fro graph G. Fails, if there
  /// was another not-popped node inserted in between.
  ///
  /// \returns True, iff the removal was successful
  static constexpr bool pop(graph_type &G, vertex_t Vtx) {
    if (size_t(Vtx) == G.Adj.size() - 1) {
      G.Adj.pop_back();
      if constexpr (!std::is_empty_v<value_type>) {
        G.Nodes.pop_back();
      }
      return true;
    }
    return false;
  }

  /// Gets the vertex-descriptor of the target-node of the given Edge
  template <typename E = edge_t>
  static constexpr std::enable_if_t<std::is_same_v<E, vertex_t>, vertex_t>
  target(edge_t Edge) noexcept {
    return Edge;
  }

  /// Copies the given edge, but replaces the target node by Tar; i.e. the
  /// weight of the returned edge and the parameter edge is same, but the target
  /// nodes may differ.
  template <typename E = edge_t>
  static constexpr std::enable_if_t<std::is_same_v<E, vertex_t>, edge_t>
  withEdgeTarget(edge_t /*edge*/, vertex_t Tar) noexcept {
    return Tar;
  }

  /// Gets the weight associated with the given edge
  static constexpr EmptyType weight(edge_t /*unused*/) noexcept { return {}; }

  /// Removes the edge denoted by It outgoing from source-vertex Vtx from the
  /// graph G. This function is not required by the is_graph_trait concept.
  ///
  /// \returns An edge_iterator directly following It that should be used to
  /// continue iteration instead of std::next(It)
  static constexpr edge_iterator removeEdge(graph_type &G, vertex_t Vtx,
                                            edge_iterator It) noexcept {
    assert(G.Adj.inbounds(Vtx));
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    assert(G.Adj[Vtx].begin() <= It && It < G.Adj[Vtx].end());
    auto Idx = std::distance(std::cbegin(G.Adj[Vtx]), It);

    std::swap(G.Adj[Vtx][Idx], G.Adj[Vtx].back());
    G.Adj[Vtx].pop_back();
    return It;
  }

  /// Removes the root denoted by It from the graph G. This function is not
  /// required by the is_graph_trait concept.
  ///
  /// \returns A roots_iterator directly following It that should be used to
  /// continue iteration instead of std::next(It)
  static constexpr roots_iterator removeRoot(graph_type &G,
                                             roots_iterator It) noexcept {
    if constexpr (!std::is_empty_v<value_type>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    assert(G.Roots.begin() <= It && It < G.Roots.end());

    auto Idx = std::distance(std::cbegin(G.Roots), It);
    std::swap(G.Roots[Idx], G.Roots.back());
    G.Roots.pop_back();
    return It;
  }
};

} // namespace psr

#endif // PHASAR_UTILS_ADJACENCYLIST_H
