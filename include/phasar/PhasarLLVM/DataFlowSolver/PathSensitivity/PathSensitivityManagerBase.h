/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGERBASE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGERBASE_H

#include "phasar/Utils/AdjacencyList.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/IntEqClasses.h"
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
    graph_type Ret{};
    if constexpr (psr::is_reservable_graph_trait_v<graph_traits_t>) {
      graph_traits_t::reserve(Ret, EquivSize);
    }

    llvm::SmallVector<vertex_t> Cache;
    Cache.resize(EquivSize, graph_traits_t::Invalid);

    llvm::SmallVector<std::pair<vertex_t, vertex_t>> WL1, WL2;

    auto *WLConsume = &WL1;
    auto *WLInsert = &WL2;

    for (auto Rt : graph_traits_t::roots(Dag)) {
      auto Eq = std::invoke(Equiv, Rt);
      if (Cache[Eq] == graph_traits_t::Invalid) {
        Cache[Eq] = graph_traits_t::addNode(
            Ret, std::move(graph_traits_t::node(Dag, Rt)));
        WLConsume->emplace_back(Rt, Cache[Eq]);
      }
    }

    size_t Depth = 0;

    while (!WLConsume->empty() && Depth < MaxDepth) {
      for (auto [Vtx, Rev] : *WLConsume) {

        for (auto Succ : graph_traits_t::outEdges(Dag, Vtx)) {
          auto SuccVtx = graph_traits_t::target(Succ);
          auto Eq = std::invoke(Equiv, SuccVtx);
          if (Cache[Eq] == graph_traits_t::Invalid) {
            Cache[Eq] = graph_traits_t::addNode(
                Ret, std::move(graph_traits_t::node(Dag, SuccVtx)));
            WLInsert->emplace_back(SuccVtx, Cache[Eq]);
          }

          auto SuccRev = Cache[Eq];
          graph_traits_t::addEdge(Ret, SuccRev,
                                  graph_traits_t::withEdgeTarget(Succ, Rev));
        }
        if (graph_traits_t::outDegree(Dag, Vtx) == 0) {
          graph_traits_t::addRoot(Ret, Rev);
        }
      }
      WLConsume->clear();

      std::swap(WLConsume, WLInsert);
      ++Depth;
    }

    for (auto [Rt, RtRev] : *WLConsume) {
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

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGERBASE_H
