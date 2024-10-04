/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_PATHSENSITIVITY_PATHSENSITIVITYMANAGERBASE_H
#define PHASAR_DATAFLOW_PATHSENSITIVITY_PATHSENSITIVITYMANAGERBASE_H

#include "phasar/Utils/AdjacencyList.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {
class Instruction;
} // namespace llvm

namespace psr {

template <typename Derived, typename AnalysisDomainTy, typename GraphType>
class PathSensitivityManagerMixin;

template <typename N> class PathSensitivityManagerBase {
public:
  using n_t = N;
  using graph_type = AdjacencyList<llvm::SmallVector<n_t, 0>>;

  static_assert(std::is_integral_v<typename GraphTraits<graph_type>::vertex_t>);

  template <typename Derived, typename AnalysisDomainTy, typename GraphType>
  friend class PathSensitivityManagerMixin;

protected:
  using graph_traits_t = GraphTraits<graph_type>;
  using vertex_t = typename graph_traits_t::vertex_t;

private:
  [[nodiscard]] bool assertIsDAG(const graph_type &Dag) const {
    llvm::BitVector Visited(graph_traits_t::size(Dag));
    llvm::DenseSet<vertex_t> CurrPath;
    CurrPath.reserve(graph_traits_t::size(Dag));

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto doAssertIsDAG = [&CurrPath, &Visited, &Dag](auto &doAssertIsDAG,
                                                     vertex_t Vtx) {
      if (!CurrPath.insert(Vtx).second) {
        PHASAR_LOG_LEVEL(ERROR, "DAG has circle: Vtx: " << uintptr_t(Vtx));
        return false;
      }

      scope_exit CurrPathPop = [&CurrPath, Vtx] { CurrPath.erase(Vtx); };
      if (Visited.test(Vtx)) {
        /// We have already analyzed this node
        /// NOTE: We must check this _after_ doing the circle check. Otherwise,
        /// that can never be true
        return true;
      }

      Visited.set(Vtx);

      for (auto Succ : graph_traits_t::outEdges(Dag, Vtx)) {
        if (!doAssertIsDAG(doAssertIsDAG, Succ)) {
          return false;
        }
      }
      return true;
    };

    for (auto Rt : graph_traits_t::roots(Dag)) {
      if (!doAssertIsDAG(doAssertIsDAG, Rt)) {
        return false;
      }
    }

    return true;
  }

  template <typename VertexTransform>
  [[nodiscard]] static graph_type
  reverseDAG(graph_type &&Dag, const VertexTransform &Equiv, size_t EquivSize,
             size_t MaxDepth) {
    using graph_traits_t = psr::GraphTraits<graph_type>;
    using vertex_t = typename graph_traits_t::vertex_t;
    graph_type Ret{};
    if constexpr (psr::is_reservable_graph_trait_v<graph_traits_t>) {
      graph_traits_t::reserve(Ret, EquivSize);
    }

    // Allocate a buffer for the temporary data needed.
    // We need:
    // - A cache of vertex_t
    // - One worklist WLConsume of pair<vertex_t, vertex_t> where we read from
    // - One worklist WLInsert of same type where we insert to

    // We iterate over the graph in levels. Each level corresponds to the
    // distance from a root node. We always have all nodes from a level inside a
    // worklist
    // -- The level we are processing is in WLConsume, the next level in
    // WLInsert. This way, we can stop the process, when we have reached the
    // MaxDepth

    constexpr auto Factor =
        sizeof(vertex_t) + 2 * sizeof(std::pair<vertex_t, vertex_t>);
    assert(EquivSize <= SIZE_MAX / Factor && "Overflow on size calculation");
    auto NumBytes = Factor * EquivSize;

    // For performance reasons, we wish to allocate the buffer on the stack, if
    // it is small enough
    static constexpr size_t MaxNumBytesInStackBuf = 8192;

    auto *Buf = NumBytes <= MaxNumBytesInStackBuf
                    ? reinterpret_cast<char *>(alloca(NumBytes))
                    : new char[NumBytes];
    std::unique_ptr<char[]> Owner; // NOLINT
    if (NumBytes > MaxNumBytesInStackBuf) {
      Owner.reset(Buf);
    }

    auto Cache = reinterpret_cast<vertex_t *>(Buf);
    std::uninitialized_fill_n(Cache, EquivSize, graph_traits_t::Invalid);

    auto *WLConsumeBegin =
        reinterpret_cast<std::pair<vertex_t, vertex_t> *>(Cache + EquivSize);
    auto *WLConsumeEnd = WLConsumeBegin;
    auto *WLInsertBegin = WLConsumeBegin + EquivSize;
    auto *WLInsertEnd = WLInsertBegin;

    for (auto Rt : graph_traits_t::roots(Dag)) {
      size_t Eq = std::invoke(Equiv, Rt);
      if (Cache[Eq] == graph_traits_t::Invalid) {
        Cache[Eq] = graph_traits_t::addNode(Ret, graph_traits_t::node(Dag, Rt));
        *WLConsumeEnd++ = {Rt, Cache[Eq]};
      }
    }

    size_t Depth = 0;

    while (WLConsumeBegin != WLConsumeEnd && Depth < MaxDepth) {
      for (auto [Vtx, Rev] : llvm::make_range(WLConsumeBegin, WLConsumeEnd)) {

        for (auto Succ : graph_traits_t::outEdges(Dag, Vtx)) {
          auto SuccVtx = graph_traits_t::target(Succ);
          size_t Eq = std::invoke(Equiv, SuccVtx);
          if (Cache[Eq] == graph_traits_t::Invalid) {
            Cache[Eq] = graph_traits_t::addNode(
                Ret, graph_traits_t::node(Dag, SuccVtx));
            *WLInsertEnd++ = {SuccVtx, Cache[Eq]};
          }

          auto SuccRev = Cache[Eq];
          graph_traits_t::addEdge(Ret, SuccRev,
                                  graph_traits_t::withEdgeTarget(Succ, Rev));
        }
        if (graph_traits_t::outDegree(Dag, Vtx) == 0) {
          graph_traits_t::addRoot(Ret, Rev);
        }
      }

      std::swap(WLConsumeBegin, WLInsertBegin);
      WLConsumeEnd = std::exchange(WLInsertEnd, WLInsertBegin);
      ++Depth;
    }

    for (auto [Rt, RtRev] : llvm::make_range(WLConsumeBegin, WLConsumeEnd)) {
      // All nodes that were cut off because they are at depth MaxDepth must
      // become roots
      graph_traits_t::addRoot(Ret, RtRev);
    }

    return Ret;
  }

  [[nodiscard]] static graph_type reverseDAG(graph_type &&Dag,
                                             size_t MaxDepth) {
    auto Sz = graph_traits_t::size(Dag);
    return reverseDAG(std::move(Dag), identity{}, Sz, MaxDepth);
  }
};

extern template class PathSensitivityManagerBase<const llvm::Instruction *>;

} // namespace psr

#endif // PHASAR_DATAFLOW_PATHSENSITIVITY_PATHSENSITIVITYMANAGERBASE_H
