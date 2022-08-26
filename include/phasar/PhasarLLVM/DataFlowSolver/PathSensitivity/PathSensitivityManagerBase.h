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

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/IntEqClasses.h"
#include "llvm/ADT/SmallVector.h"

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
  bool assertIsDAG(const graph_type &Dag) const {
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

  graph_type reverseDAG(graph_type &&Dag, size_t MaxDepth) const {
    struct ReverseDAGContext {
      llvm::BitVector Visited;
      size_t CurrDepth = 0;
      size_t MaxDepth = 0;
    } Ctx;

    Ctx.Visited.resize(graph_traits_t::size(Dag));
    Ctx.MaxDepth = MaxDepth;

    graph_type Ret{};
    if constexpr (is_reservable_graph_trait_v<graph_traits_t>) {
      graph_traits_t::reserve(Ret, graph_traits_t::size(Dag));
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto buildReverseDag = [&Ctx, &Ret, &Dag](auto &buildReverseDag,
                                              vertex_t Vtx) {
      if (Ctx.Visited.test(Vtx)) {
        return Vtx;
      }

      Ctx.Visited.set(Vtx);

      auto Rev = graph_traits_t::addNode(
          Ret, std::move(graph_traits_t::node(Dag, Vtx)));

      if (Ctx.CurrDepth >= Ctx.MaxDepth) {
        graph_traits_t::addRoot(Ret, Rev);
        return Rev;
      }

      ++Ctx.CurrDepth;
      scope_exit DecreaseDepth = [&Ctx] { --Ctx.CurrDepth; };

      for (auto Succ : graph_traits_t::outEdges(Dag, Vtx)) {
        /// NOTE: Depending on the depth of SuccRev, we can still get DAGs
        /// deeper than MaxDepth!
        /// However, this is not considered harmful as of now - the DAG still
        /// does not exceed a particular program-slice which size is fixed
        auto SuccRev =
            buildReverseDag(buildReverseDag, graph_traits_t::target(Succ));
        graph_traits_t::addEdge(Ret, SuccRev,
                                graph_traits_t::withEdgeTarget(Succ, Rev));
      }

      if (graph_traits_t::outDegree(Dag, Vtx) == 0) {
        graph_traits_t::addRoot(Ret, Rev);
      }

      return Rev;
    };

    for (auto Rt : graph_traits_t::roots(Dag)) {
      buildReverseDag(buildReverseDag, Rt);
    }

    return Ret;
  }

  graph_type reverseDAG(graph_type &&Dag, const llvm::IntEqClasses &Equiv,
                        size_t MaxDepth) const {

    struct ReverseDAGContext {
      llvm::SmallVector<vertex_t> Cache;
      size_t CurrDepth = 0;
      size_t MaxDepth = 0;
    } Ctx;

    Ctx.Cache.resize(Equiv.getNumClasses(), graph_traits_t::Invalid);
    Ctx.MaxDepth = MaxDepth;

    graph_type Ret{};
    // Ret.Dag = &Dag;
    // Ret.Leaf = Equiv[Dag.Root];
    if constexpr (is_reservable_graph_trait_v<graph_traits_t>) {
      graph_traits_t::reserve(Ret, Equiv.getNumClasses());
    }
    // Ret.Adj.reserve(Equiv.getNumClasses());
    // Ret.Rev2Vtx.reserve(Equiv.getNumClasses());

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto buildReverseDag = [&Ctx, &Ret, &Equiv, &Dag](auto &buildReverseDag,
                                                      vertex_t Vtx) {
      auto Eq = Equiv[Vtx];
      if (Ctx.Cache[Eq] != graph_traits_t::Invalid) {
        return Ctx.Cache[Eq];
      }

      // typename ReverseDAG::vertex_t Rev = Ret.size();
      // Ret.Rev2Vtx.push_back(Vtx);
      // Ret.Adj.emplace_back();
      auto Rev = graph_traits_t::addNode(
          Ret, std::move(graph_traits_t::node(Dag, Vtx)));
      Ctx.Cache[Eq] = Rev;

      if (Ctx.CurrDepth >= Ctx.MaxDepth) {
        // PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
        //                      "Reached MaxDepth: " << Ctx.CurrDepth);
        graph_traits_t::addRoot(Ret, Rev);
        // Ret.Roots.push_back(Rev);
        return Rev;
      }
      // else {
      //   PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
      //                        "Have not reached MaxDepth: "
      //                            << Ctx.CurrDepth << " vs " << Ctx.MaxDepth);
      // }

      ++Ctx.CurrDepth;
      scope_exit DecreaseDepth = [&Ctx] { --Ctx.CurrDepth; };

      for (auto Succ : graph_traits_t::outEdges(Dag, Vtx)) {
        /// NOTE: Depending on the depth of SuccRev, we can still get DAGs
        /// deeper than MaxDepth!
        /// However, this is not considered harmful as of now - the DAG still
        /// does not exceed a particular program-slice which size is fixed
        auto SuccRev =
            buildReverseDag(buildReverseDag, graph_traits_t::target(Succ));
        graph_traits_t::addEdge(Ret, SuccRev,
                                graph_traits_t::withEdgeTarget(Succ, Rev));
        // Ret.Adj[SuccRev].push_back(Rev);
      }

      if (graph_traits_t::outDegree(Dag, Vtx) == 0) {
        graph_traits_t::addRoot(Ret, Rev);
        // Ret.Roots.push_back(Rev);
      }

      return Rev;
    };

    for (auto Rt : graph_traits_t::roots(Dag)) {
      buildReverseDag(buildReverseDag, Rt);
    }

    return Ret;
  }
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGERBASE_H
