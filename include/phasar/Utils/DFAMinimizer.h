/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#ifndef PHASAR_UTILS_DFAMINIMIZER_H
#define PHASAR_UTILS_DFAMINIMIZER_H

#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/IntEqClasses.h"
#include "llvm/ADT/SmallVector.h"

namespace psr {

template <typename GraphTy>
[[nodiscard]] std::decay_t<GraphTy>
createEquivalentGraphFrom(GraphTy &&G, const llvm::IntEqClasses &Eq)
#if __cplusplus >= 202002L
    requires is_graph<GraphTy>
#endif
{
  using traits_t = GraphTraits<GraphTy>;
  using vertex_t = typename traits_t::vertex_t;

  std::decay_t<GraphTy> Ret;

  llvm::SmallVector<std::pair<vertex_t, vertex_t>, 0> Cache(
      Eq.getNumClasses(), {traits_t::Invalid, traits_t::Invalid});

  if constexpr (is_reservable_graph_trait_v<traits_t>) {
    traits_t::reserve(Ret, Eq.getNumClasses());
  }

  for (auto Rt : traits_t::roots(G)) {
    size_t EqVtx = Eq[Rt];
    if (Cache[EqVtx].second != traits_t::Invalid) {
      continue;
    }
    auto EqRt =
        traits_t::addNode(Ret, forward_like<GraphTy>(traits_t::node(G, Rt)));

    Cache[EqVtx] = {Rt, EqRt};
    traits_t::addRoot(Ret, EqRt);
  }
  for (auto Vtx : traits_t::vertices(G)) {
    size_t EqVtx = Eq[Vtx];
    if (Cache[EqVtx].second != traits_t::Invalid) {
      continue;
    }
    Cache[EqVtx] = {Vtx, traits_t::addNode(Ret, forward_like<GraphTy>(
                                                    traits_t::node(G, Vtx)))};
  }

  for (auto [Vtx, EqVtx] : Cache) {
    assert(Vtx != traits_t::Invalid);
    assert(EqVtx != traits_t::Invalid);
    for (auto Succ : traits_t::outEdges(G, Vtx)) {
      auto EqSucc = Eq[traits_t::target(Succ)];
      traits_t::addEdge(Ret, EqVtx, traits_t::withEdgeTarget(Succ, EqSucc));
    }
    traits_t::dedupOutEdges(Ret, EqVtx);
  }

  return Ret;
}

template <typename GraphTy>
[[nodiscard]] llvm::IntEqClasses minimizeGraph(const GraphTy &G)
#if __cplusplus >= 202002L
    requires is_graph<GraphTy>
#endif
{

  using traits_t = GraphTraits<GraphTy>;
  using vertex_t = typename traits_t::vertex_t;
  using edge_t = typename traits_t::edge_t;

  auto DagSize = traits_t::size(G);
  llvm::SmallVector<std::pair<vertex_t, vertex_t>> WorkList;
  WorkList.reserve(DagSize);

  const auto &Vertices = traits_t::vertices(G);

  for (auto VtxBegin = Vertices.begin(), VtxEnd = Vertices.end();
       VtxBegin != VtxEnd; ++VtxBegin) {
    for (auto Inner = std::next(VtxBegin); Inner != VtxEnd; ++Inner) {
      if (traits_t::node(G, *VtxBegin) == traits_t::node(G, *Inner) &&
          traits_t::outDegree(G, *VtxBegin) == traits_t::outDegree(G, *Inner)) {
        WorkList.emplace_back(*VtxBegin, *Inner);
      }
    }
  }

  size_t Idx = 0;

  llvm::IntEqClasses Equiv(traits_t::size(G));

  auto isEquivalent = [&Equiv](edge_t LHS, edge_t RHS) {
    if (traits_t::weight(LHS) != traits_t::weight(RHS)) {
      return false;
    }

    if (traits_t::target(LHS) == traits_t::target(RHS)) {
      return true;
    }

    return Equiv.findLeader(traits_t::target(LHS)) ==
           Equiv.findLeader(traits_t::target(RHS));
  };

  auto makeEquivalent = [&Equiv](vertex_t LHS, vertex_t RHS) {
    if (LHS == RHS) {
      return;
    }

    Equiv.join(LHS, RHS);
  };

  auto removeAt = [&WorkList](size_t I) {
    std::swap(WorkList[I], WorkList.back());
    WorkList.pop_back();
    return I - 1;
  };

  /// NOTE: This algorithm can be further optimized, but for now it is fast
  /// enough

  bool Changed = true;
  while (Changed) {
    Changed = false;
    for (size_t I = 0; I < WorkList.size(); ++I) {
      auto [LHS, RHS] = WorkList[I];
      bool Eq = true;
      for (auto [LSucc, RSucc] :
           llvm::zip(traits_t::outEdges(G, LHS), traits_t::outEdges(G, RHS))) {
        if (!isEquivalent(traits_t::target(LSucc), traits_t::target(RSucc))) {
          Eq = false;
          break;
        }
      }

      if (Eq) {
        makeEquivalent(LHS, RHS);
        I = removeAt(I);
        Changed = true;
        continue;
      }

      if (traits_t::outDegree(G, LHS) == 2) {
        auto LFirst = *traits_t::outEdges(G, LHS).begin();
        auto LSecond = *std::next(traits_t::outEdges(G, LHS).begin());
        auto RFirst = *traits_t::outEdges(G, RHS).begin();
        auto RSecond = *std::next(traits_t::outEdges(G, RHS).begin());

        if (isEquivalent(LFirst, RSecond) && isEquivalent(LSecond, RFirst)) {
          makeEquivalent(LHS, RHS);
          I = removeAt(I);
          Changed = true;
          continue;
        }
      }
    }
  }

  Equiv.compress();

  PHASAR_LOG_LEVEL_CAT(DEBUG, "GraphTraits",
                       "> Computed " << Equiv.getNumClasses()
                                     << " Equivalence classes for " << DagSize
                                     << " nodes");

  return Equiv;
}

} // namespace psr

#endif // PHASAR_UTILS_DFAMINIMIZER_H
