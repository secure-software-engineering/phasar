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

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/SmallVector.h"

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

  static inline constexpr auto Invalid = std::numeric_limits<vertex_t>::max();

  template <typename TT,
            typename = std::enable_if_t<!std::is_same_v<TT, llvm::NoneType>>>
  static vertex_t addNode(graph_type &G, TT &&Val) {
    assert(G.Adj.size() == G.Nodes.size());

    auto Ret = G.Nodes.size();
    G.Nodes.push_back(std::forward<TT>(Val));
    G.Adj.emplace_back();
    return Ret;
  }

  template <typename TT = value_type,
            typename = std::enable_if_t<std::is_same_v<TT, llvm::NoneType>>>
  static vertex_t addNode(graph_type &G, llvm::NoneType /*Val*/ = llvm::None) {
    auto Ret = G.Adj.size();
    G.Adj.emplace_back();
    return Ret;
  }

  static void addRoot(graph_type &G, vertex_t Vtx) {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(Vtx < G.Nodes.size());
      assert(G.Adj.size() == G.Nodes.size());
    }
    G.Roots.push_back(Vtx);
  }

  static llvm::ArrayRef<vertex_t> roots(const graph_type &G) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Roots;
  }

  static void addEdge(graph_type &G, vertex_t From, edge_t To) {
    assert(From < G.Adj.size());
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(B.Adj.size() == G.Nodes.size());
    }
    G.Adj[From].push_back(std::move(To));
  }

  static llvm::ArrayRef<edge_t> outEdges(const graph_type &G,
                                         vertex_t Vtx) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(Vtx < G.Nodes.size());
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Adj[Vtx];
  }

  static size_t outDegree(const graph_type &G, vertex_t Vtx) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(Vtx < G.Nodes.size());
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Adj[Vtx].size();
  }

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

  template <typename TT = value_type,
            typename = std::enable_if_t<!std::is_same_v<TT, llvm::NoneType>>>
  static llvm::ArrayRef<value_type> nodes(const graph_type &G) noexcept {
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes;
  }
  template <typename TT = value_type>
  static std::enable_if_t<!std::is_same_v<TT, llvm::NoneType>,
                          llvm::MutableArrayRef<value_type>>
  nodes(graph_type &G) noexcept {
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes;
  }
  template <typename TT = value_type,
            typename = std::enable_if_t<std::is_same_v<TT, llvm::NoneType>>>
  static std::enable_if_t<std::is_same_v<TT, llvm::NoneType>,
                          RepeatRangeType<value_type>>
  nodes(const graph_type &G) noexcept {
    return repeat(llvm::None, G.Adj.size());
  }

  static auto vertices(const graph_type &G) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return psr::iota(0, G.Adj.size());
  }

  template <typename TT = value_type,
            typename = std::enable_if_t<!std::is_same_v<TT, llvm::NoneType>>>
  static const value_type &node(const graph_type &G, vertex_t Vtx) noexcept {
    assert(Vtx < G.Nodes.size());
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes[Vtx];
  }
  template <typename TT = value_type,
            typename = std::enable_if_t<!std::is_same_v<TT, llvm::NoneType>>>
  static value_type &node(graph_type &G, vertex_t Vtx) noexcept {
    assert(Vtx < G.Nodes.size());
    assert(G.Adj.size() == G.Nodes.size());
    return G.Nodes[Vtx];
  }

  template <typename TT = value_type,
            typename = std::enable_if_t<std::is_same_v<TT, llvm::NoneType>>>
  static llvm::NoneType node([[maybe_unused]] const graph_type &G,
                             [[maybe_unused]] vertex_t Vtx) noexcept {
    assert(Vtx < G.Adj.size());
    return llvm::None;
  }

  static size_t size(const graph_type &G) noexcept {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
    }
    return G.Adj.size();
  }

  static void reserve(graph_type &G, size_t Capacity) {
    if constexpr (!std::is_same_v<value_type, llvm::NoneType>) {
      assert(G.Adj.size() == G.Nodes.size());
      G.Nodes.reserve(Capacity);
    }

    G.Adj.reserve(Capacity);
  }

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

  template <typename E = edge_t>
  static std::enable_if_t<std::is_same_v<E, vertex_t>, vertex_t>
  target(edge_t Edge) noexcept {
    return Edge;
  }

  template <typename E = edge_t>
  static std::enable_if_t<std::is_same_v<E, vertex_t>, edge_t>
  withEdgeTarget(edge_t /*edge*/, vertex_t Tar) noexcept {
    return Tar;
  }

  static llvm::NoneType weight(edge_t /*unused*/) noexcept {
    return llvm::None;
  }

#if __cplusplus >= 202002L
  static_assert(is_graph<AdjacencyList<T>>);
  static_assert(is_reservable_graph_trait<GraphTraits<AdjacencyList<T>>>);
#endif
};

} // namespace psr

#endif // PHASAR_UTILS_ADJACENCYLIST_H
