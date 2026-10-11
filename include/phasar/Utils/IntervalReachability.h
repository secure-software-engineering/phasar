#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/CsrGraph.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/SCCGeneric.h"
#include "phasar/Utils/SCCId.h"
#include "phasar/Utils/StrongTypeDef.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>

/// Position of a target vertex in an IntervalReachabilityIndex.
PHASAR_STRONG_TYPEDEF(psr, uint32_t, ReachRank, None = UINT32_MAX);
/// Id of a set of rank intervals in an IntervalReachabilityIndex.
PHASAR_STRONG_TYPEDEF(psr, uint32_t, IntervalSetId, Empty = 0);

namespace psr {

/// Half-open interval [Lo, Hi) of ranks.
struct RankInterval {
  [[nodiscard]] constexpr bool empty() const noexcept { return Lo >= Hi; }
  [[nodiscard]] constexpr size_t size() const noexcept {
    return empty() ? 0 : to_underlying(Hi) - to_underlying(Lo);
  }
  [[nodiscard]] constexpr bool contains(ReachRank Rank) const noexcept {
    return Lo <= Rank && Rank < Hi;
  }

  [[nodiscard]] friend constexpr bool
  operator==(RankInterval Lhs, RankInterval Rhs) noexcept = default;

  ReachRank Lo{};
  ReachRank Hi{};
};

template <SmallIdType VtxId> class IntervalReachabilityBuilder;

/// Reachability index over a directed graph: answers whether a vertex reaches
/// a target vertex in O(log k), where k is the number of rank intervals of the
/// source (usually small).
///
/// Built by IntervalReachabilityBuilder
template <SmallIdType VtxId> class IntervalReachabilityIndex {
  friend class IntervalReachabilityBuilder<VtxId>;

public:
  using SCCIdTy = SCCId<VtxId>;

  static constexpr SCCIdTy NoSCC = SCCIdTy(UINT32_MAX);

  [[nodiscard]] size_t numVertices() const noexcept { return SCCOf.size(); }
  [[nodiscard]] size_t numTargets() const noexcept {
    return RankToVertex.size();
  }
  [[nodiscard]] size_t numSCCs() const noexcept { return SCCRange.size(); }

  /// Whether Vtx was reachable from the roots at build time.
  [[nodiscard]] bool isIndexed(VtxId Vtx) const noexcept {
    return SCCOf[Vtx] != NoSCC;
  }

  // A sink node
  [[nodiscard]] bool isTarget(VtxId Vtx) const noexcept {
    return RankOf[Vtx] != ReachRank::None;
  }

  /// The rank of Vtx, or ReachRank::None if Vtx is not a target.
  [[nodiscard]] ReachRank rankOf(VtxId Vtx) const noexcept {
    return RankOf[Vtx];
  }

  [[nodiscard]] VtxId vertexOfRank(ReachRank Rank) const noexcept {
    return RankToVertex[Rank];
  }

  /// The targets covered by the given rank interval.
  [[nodiscard]] llvm::ArrayRef<VtxId>
  targetsIn(RankInterval Interval) const noexcept {
    if (Interval.empty()) {
      return {};
    }
    return {&RankToVertex[Interval.Lo], Interval.size()};
  }

  /// The strongly connected component of Vtx, or NoSCC if Vtx is not indexed.
  /// SCC ids are in reverse topological order: an SCC only reaches SCCs with
  /// smaller or equal id.
  [[nodiscard]] SCCIdTy sccOf(VtxId Vtx) const noexcept { return SCCOf[Vtx]; }

  /// Whether From reaches the target To. False, if From is not indexed or To
  /// is not a target.
  [[nodiscard]] bool reaches(VtxId From, VtxId To) const noexcept {
    auto SCC = SCCOf[From];
    auto Rank = RankOf[To];
    if (SCC == NoSCC || Rank == ReachRank::None) {
      return false;
    }
    return sccReaches(SCC, Rank);
  }

  /// Whether the given SCC reaches the target with the given rank.
  [[nodiscard]] bool sccReaches(SCCIdTy SCC, ReachRank Rank) const noexcept {
    if (SCCRange[SCC].contains(Rank)) {
      return true;
    }

    auto Extras = extrasOf(SCC);
    const auto *It = std::upper_bound(
        Extras.begin(), Extras.end(), Rank,
        [](ReachRank R, RankInterval Interval) { return R < Interval.Lo; });
    return It != Extras.begin() && Rank < std::prev(It)->Hi;
  }

  /// Calls Handler for each interval of target ranks reachable from From, in
  /// ascending order. The intervals are disjoint and non-empty.
  void forEachReachableRange(VtxId From,
                             std::invocable<RankInterval> auto Handler) const {
    auto SCC = SCCOf[From];
    if (SCC == NoSCC) {
      return;
    }
    for (auto Interval : extrasOf(SCC)) {
      std::invoke(Handler, Interval);
    }
    if (!SCCRange[SCC].empty()) {
      std::invoke(Handler, SCCRange[SCC]);
    }
  }

  /// Calls Handler for each target reachable from From, in rank order.
  void forEachReachableTarget(VtxId From,
                              std::invocable<VtxId> auto Handler) const {
    forEachReachableRange(From, [&](RankInterval Interval) {
      for (auto Vtx : targetsIn(Interval)) {
        std::invoke(Handler, Vtx);
      }
    });
  }

  /// Number of rank intervals describing the targets reachable from SCC.
  [[nodiscard]] size_t numIntervalsOf(SCCIdTy SCC) const noexcept {
    return extrasOf(SCC).size() + size_t(!SCCRange[SCC].empty());
  }

  [[nodiscard]] size_t getApproxSizeInBytes() const noexcept {
    return SCCOf.capacity() * sizeof(SCCIdTy) +
           RankOf.capacity() * sizeof(ReachRank) +
           RankToVertex.capacity() * sizeof(VtxId) +
           SCCRange.capacity() * sizeof(RankInterval) +
           SCCExtras.capacity() * sizeof(IntervalSetId) +
           IntervalPool.capacity_in_bytes() +
           SetOffsets.capacity() * sizeof(uint32_t);
  }

private:
  [[nodiscard]] llvm::ArrayRef<RankInterval>
  extrasOf(SCCIdTy SCC) const noexcept {
    return csrSuccs(SetOffsets, IntervalPool, SCCExtras[SCC]);
  }

  void reset(size_t NumVertices) {
    SCCOf.assign(NumVertices, NoSCC);
    RankOf.assign(NumVertices, ReachRank::None);
    RankToVertex.clear();
    SCCRange.clear();
    SCCExtras.clear();
    IntervalPool.clear();
    SetOffsets.clear();
    // IntervalSetId::Empty
    SetOffsets.push_back(0);
    SetOffsets.push_back(0);
  }

  TypedVector<VtxId, SCCIdTy> SCCOf;
  TypedVector<VtxId, ReachRank> RankOf;
  TypedVector<ReachRank, VtxId> RankToVertex;

  /// Ranks of the targets in the DFS subtree of the SCC's root
  TypedVector<SCCIdTy, RankInterval> SCCRange;
  /// Targets reachable via non-tree edges. Their intervals all lie below
  /// SCCRange[SCC].Lo: targets with higher ranks are discovered after the SCC
  /// is complete, so they are not reachable from it.
  TypedVector<SCCIdTy, IntervalSetId> SCCExtras;

  /// Interval sets in CSR format
  llvm::SmallVector<RankInterval, 0> IntervalPool;
  TypedVector<IntervalSetId, uint32_t> SetOffsets;
};

/// Computes an IntervalReachabilityIndex during a single DFS (Pearce's SCC
/// algorithm, see visitSCCsFrom()).
///
/// Owns scratch buffers that are reused across build() calls; keep the builder
/// alive when indexing many (small) graphs.
///
/// Complexity: O(V + E + total size of the merged interval sets).
template <SmallIdType VtxId> class IntervalReachabilityBuilder {
public:
  using IndexTy = IntervalReachabilityIndex<VtxId>;
  using SCCIdTy = typename IndexTy::SCCIdTy;

  /// Indexes all vertices of Graph reachable from Roots, all of them as
  /// targets. The ranks are then DFS pre-order numbers.
  template <is_const_graph G>
    requires std::same_as<typename GraphTraits<G>::vertex_t, VtxId>
  [[nodiscard]] IndexTy build(const G &Graph, llvm::ArrayRef<VtxId> Roots) {
    return build(Graph, Roots, /*IsTarget=*/psr::TrueFn{});
  }

  template <is_const_graph G>
    requires std::same_as<typename GraphTraits<G>::vertex_t, VtxId>
  void build(const G &Graph, llvm::ArrayRef<VtxId> Roots, IndexTy &Out) {
    build(Graph, Roots, Out, /*IsTarget=*/psr::TrueFn{});
  }

  /// Indexes all vertices of Graph reachable from Roots, only those
  /// satisfying IsTarget as targets.
  ///
  /// Select few targets if only reachability to those is of interest: the
  /// interval sets shrink accordingly.
  template <is_const_graph G>
    requires std::same_as<typename GraphTraits<G>::vertex_t, VtxId>
  void build(const G &Graph, llvm::ArrayRef<VtxId> Roots, IndexTy &Out,
             std::predicate<VtxId> auto IsTarget) {
    auto NumVertices = GraphTraits<G>::size(Graph);
    Out.reset(NumVertices);
    SCCData.reset(NumVertices);
    RankBegin.resize(NumVertices);
    SeenBy.clear();

    const auto OnDiscover = [&](VtxId Vtx) {
      auto Rank = ReachRank(Out.RankToVertex.size());
      RankBegin[Vtx] = Rank;
      if (std::invoke(IsTarget, Vtx)) {
        Out.RankOf[Vtx] = Rank;
        Out.RankToVertex.push_back(Vtx);
      }
    };
    const auto OnSCC = [&](SCCIdTy SCC, llvm::ArrayRef<VtxId> Members) {
      completeSCC(Graph, SCC, Members, Out);
    };

    for (auto Root : Roots) {
      if (!SCCData.isVisited(Root)) {
        visitSCCsFrom(Graph, Root, SCCData, OnDiscover, OnSCC);
      }
    }
  }

  /// Indexes all vertices of Graph reachable from Roots, only those
  /// satisfying IsTarget as targets.
  template <is_const_graph G>
    requires std::same_as<typename GraphTraits<G>::vertex_t, VtxId>
  [[nodiscard]] IndexTy build(const G &Graph, llvm::ArrayRef<VtxId> Roots,
                              std::predicate<VtxId> auto IsTarget) {
    IndexTy Out{};
    build(Graph, Roots, Out, copyOrRef(IsTarget));
    return Out;
  }

private:
  template <typename G>
  void completeSCC(const G &Graph, SCCIdTy SCC, llvm::ArrayRef<VtxId> Members,
                   IndexTy &Out) {
    assert(size_t(SCC) == Out.SCCRange.size());

    // Members.back() is the root of the SCC in the DFS tree. Its subtree is
    // completely discovered.
    auto Range = RankInterval{
        .Lo = RankBegin[Members.back()],
        .Hi = ReachRank(Out.RankToVertex.size()),
    };

    for (auto Member : Members) {
      Out.SCCOf[Member] = SCC;
    }
    Out.SCCRange.push_back(Range);
    SeenBy.push_back(SCC);

    // All successor SCCs are complete, as SCCs complete in reverse
    // topological order
    SuccSCCs.clear();
    for (auto Member : Members) {
      for (const auto &Edge : GraphTraits<G>::outEdges(Graph, Member)) {
        auto SuccSCC = Out.SCCOf[GraphTraits<G>::target(Edge)];
        if (SuccSCC == SCC || SeenBy[SuccSCC] == SCC) {
          continue;
        }
        SeenBy[SuccSCC] = SCC;
        SuccSCCs.push_back(SuccSCC);
      }
    }

    Out.SCCExtras.push_back(mergeExtras(Range.Lo, Out));
  }

  /// Computes the extras-set of an SCC whose range starts at Lo from the
  /// interval sets of its successor SCCs.
  ///
  /// The range of a successor SCC lies either inside the own range (DFS-tree
  /// descendant) or entirely below Lo (cross edge). Successor extras at or
  /// above Lo are covered by the own range, so they are clipped at Lo.
  [[nodiscard]] IntervalSetId mergeExtras(ReachRank Lo, IndexTy &Out) {
    const auto IsCrossRange = [Lo](RankInterval Range) {
      return !Range.empty() && Range.Hi <= Lo;
    };

    // Fast path for chains and loop bodies: a single contributing descendant,
    // whose extras need no clipping
    size_t NumContributing = 0;
    std::optional<IntervalSetId> Reusable;
    for (auto SuccSCC : SuccSCCs) {
      bool IsCross = IsCrossRange(Out.SCCRange[SuccSCC]);
      auto Extras = Out.extrasOf(SuccSCC);
      if (!IsCross && Extras.empty()) {
        continue;
      }

      ++NumContributing;
      Reusable = std::nullopt;
      if (!IsCross && Extras.back().Hi <= Lo) {
        Reusable = Out.SCCExtras[SuccSCC];
      }
    }
    if (NumContributing == 0) {
      return IntervalSetId::Empty;
    }
    if (NumContributing == 1 && Reusable) {
      return *Reusable;
    }

    Candidates.clear();
    for (auto SuccSCC : SuccSCCs) {
      auto Range = Out.SCCRange[SuccSCC];
      if (IsCrossRange(Range)) {
        Candidates.push_back(Range);
      }
      for (auto Interval : Out.extrasOf(SuccSCC)) {
        if (Interval.Lo >= Lo) {
          break;
        }
        Candidates.push_back({Interval.Lo, std::min(Interval.Hi, Lo)});
      }
    }

    llvm::sort(Candidates, [](RankInterval Lhs, RankInterval Rhs) {
      return Lhs.Lo < Rhs.Lo;
    });

    auto Ret = IntervalSetId(Out.SetOffsets.size() - 1);
    for (auto Interval : Candidates) {
      // Coalesce overlapping and adjacent intervals
      if (Out.IntervalPool.size() != Out.SetOffsets.asRef().back() &&
          Interval.Lo <= Out.IntervalPool.back().Hi) {
        Out.IntervalPool.back().Hi =
            std::max(Out.IntervalPool.back().Hi, Interval.Hi);
        continue;
      }
      Out.IntervalPool.push_back(Interval);
    }
    Out.SetOffsets.push_back(uint32_t(Out.IntervalPool.size()));
    return Ret;
  }

  PearceSCCData<VtxId> SCCData;
  TypedVector<VtxId, ReachRank> RankBegin;

  /// The last SCC that has seen this SCC as successor
  TypedVector<SCCIdTy, SCCIdTy> SeenBy;

  llvm::SmallVector<SCCIdTy, 0> SuccSCCs;
  llvm::SmallVector<RankInterval, 0> Candidates;
};

} // namespace psr
