#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/EmptyBaseOptimizationUtils.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/RepeatIterator.h"
#include "phasar/Utils/StrongTypeDef.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"

#include <cassert>
#include <cstdint>
#include <numeric>
#include <utility>

namespace psr {

template <IdType IndexId, typename VtxId>
[[nodiscard]] inline llvm::ArrayRef<VtxId>
csrSuccs(const TypedVector<IndexId, uint32_t> &Offsets,
         const llvm::SmallVectorImpl<VtxId> &Targets, IndexId Vtx) {
  auto Start = Offsets[Vtx];
  auto End = Offsets[nextId(Vtx)];
  return llvm::ArrayRef(Targets).slice(Start, End - Start);
}

template <IdType IndexId, typename VtxId>
[[nodiscard]] inline llvm::MutableArrayRef<VtxId>
csrSuccsMut(const TypedVector<IndexId, uint32_t> &Offsets,
            llvm::SmallVectorImpl<VtxId> &Targets, IndexId Vtx) {
  auto Start = Offsets[Vtx];
  auto End = Offsets[nextId(Vtx)];
  return llvm::MutableArrayRef(Targets).slice(Start, End - Start);
}

/// Immutable directed graph in compressed-sparse-row format: the successors of
/// vertex V are Targets[Offsets[V] .. Offsets[V+1]).
///
/// Build it with fromEdges() or assignEdges(). assignEdges() reuses the
/// allocated storage, so a single CsrGraph object can serve as buffer for many
/// short-lived graphs.
template <IdType VtxId> class CsrGraph {
public:
  /// Builds the graph from (Source, Target) pairs. The successors of each
  /// vertex keep the relative order of Edges.
  [[nodiscard]] static CsrGraph
  fromEdges(size_t NumVertices, llvm::ArrayRef<std::pair<VtxId, VtxId>> Edges,
            llvm::ArrayRef<VtxId> RootVtxs = {}) {
    CsrGraph Ret;
    Ret.assignEdges(NumVertices, Edges, RootVtxs);
    return Ret;
  }

  /// Returns G with all edges inverted. The predecessors of each vertex are
  /// sorted ascending. The roots of the result are the vertices without
  /// successors in G.
  [[nodiscard]] static CsrGraph reversed(const CsrGraph &G) {
    CsrGraph Ret;
    Ret.countEdges(G.numVertices(), G.Targets);
    Ret.Targets.resize_for_overwrite(G.numEdges());
    for (auto From : llvm::reverse(iota<VtxId>(G.numVertices()))) {
      for (auto To : G.succsOf(From)) {
        Ret.Targets[--Ret.Offsets[To]] = From;
      }
    }
    for (auto Vtx : iota<VtxId>(G.numVertices())) {
      if (G.succsOf(Vtx).empty()) {
        Ret.Roots.push_back(Vtx);
      }
    }
    return Ret;
  }

  /// Replaces the contents of this graph by NumVertices vertices and the
  /// given (Source, Target) edges. Clears the roots.
  void assignEdges(size_t NumVertices,
                   llvm::ArrayRef<std::pair<VtxId, VtxId>> Edges,
                   llvm::ArrayRef<VtxId> RootVtxs = {}) {
    countEdges(NumVertices, llvm::make_first_range(Edges));
    Targets.resize_for_overwrite(Edges.size());
    for (auto [From, To] : llvm::reverse(Edges)) {
      assert(size_t(To) < NumVertices);
      Targets[--Offsets[From]] = To;
    }
    Roots.assign(RootVtxs.begin(), RootVtxs.end());
  }

  [[nodiscard]] size_t numVertices() const noexcept {
    assert(!Offsets.empty());
    return Offsets.size() - 1;
  }

  [[nodiscard]] size_t numEdges() const noexcept { return Targets.size(); }

  [[nodiscard]] llvm::ArrayRef<VtxId> succsOf(VtxId Vtx) const noexcept {
    return csrSuccs(Offsets, Targets, Vtx);
  }

  [[nodiscard]] llvm::ArrayRef<VtxId> roots() const & noexcept { return Roots; }
  [[nodiscard]] auto roots() && noexcept { return Roots; }

private:
  /// Sets Offsets[V] to the end of the successors of V. Inserting the edges
  /// at --Offsets[From] in reverse order then leaves Offsets[V] at the begin.
  void countEdges(size_t NumVertices, auto &&Sources) {
    Offsets.assign(NumVertices + 1, 0);
    for (auto From : Sources) {
      ++Offsets[From];
    }
    std::partial_sum(Offsets.begin(), Offsets.end(), Offsets.begin());
  }

  /// One entry per vertex plus a sentinel holding numEdges()
  TypedVector<VtxId, uint32_t> Offsets{0};
  llvm::SmallVector<VtxId, 0> Targets;
  llvm::SmallVector<VtxId, 1> Roots;
};

template <IdType VtxId> struct GraphTraits<CsrGraph<VtxId>> {
  using graph_type = CsrGraph<VtxId>;
  using value_type = EmptyType;
  using vertex_t = VtxId;
  using edge_t = VtxId;

  static constexpr auto Invalid = VtxId(UINT32_MAX);

  [[nodiscard]] static llvm::ArrayRef<edge_t> outEdges(const graph_type &G,
                                                       vertex_t Vtx) noexcept {
    return G.succsOf(Vtx);
  }

  [[nodiscard]] static size_t outDegree(const graph_type &G,
                                        vertex_t Vtx) noexcept {
    return G.succsOf(Vtx).size();
  }

  [[nodiscard]] static auto nodes(const graph_type &G) noexcept {
    return repeat(EmptyType{}, G.numVertices());
  }

  [[nodiscard]] static llvm::ArrayRef<vertex_t>
  roots(const graph_type &G) noexcept {
    return G.roots();
  }

  [[nodiscard]] static auto vertices(const graph_type &G) noexcept {
    return iota<vertex_t>(G.numVertices());
  }

  [[nodiscard]] static value_type node(const graph_type & /*G*/,
                                       vertex_t /*Vtx*/) noexcept {
    return {};
  }

  [[nodiscard]] static size_t size(const graph_type &G) noexcept {
    return G.numVertices();
  }

  [[nodiscard]] static size_t
  roots_size(const graph_type &G) noexcept { // NOLINT
    return G.roots().size();
  }

  [[nodiscard]] static constexpr vertex_t target(edge_t Edge) noexcept {
    return Edge;
  }

  [[nodiscard]] static constexpr edge_t withEdgeTarget(edge_t /*Edge*/,
                                                       vertex_t Tar) noexcept {
    return Tar;
  }
};

} // namespace psr
