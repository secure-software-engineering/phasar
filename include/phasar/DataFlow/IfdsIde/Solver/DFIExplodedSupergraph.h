#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/DFISolverResults.h"
#include "phasar/Utils/BitSet.h"
#include "phasar/Utils/CsrGraph.h"
#include "phasar/Utils/FunctionId.h"
#include "phasar/Utils/IntervalReachability.h"
#include "phasar/Utils/SCCGeneric.h"
#include "phasar/Utils/SCCId.h"
#include "phasar/Utils/StrongTypeDef.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"

#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>

/// A function entry <StartPoint, Fact> reached by at least one call
PHASAR_STRONG_TYPEDEF(psr::dfi, uint32_t, EntryId);
/// Index of a vertex among the vertices of its function
PHASAR_STRONG_TYPEDEF(psr::dfi, uint32_t, LocalVertexId);

namespace psr::detail {

/// The exploded supergraph under construction by the DFISolver.
///
/// Works on ids only; evaluating flow functions is up to the solver. Tracks
/// which vertices still need to be expanded and which functions need to be
/// summarized, and schedules this work callees first.
class DFIExplodedSupergraph {
public:
  enum class ActionKind { Expand, Summarize };

  struct Action {
    FunctionId Fun{};
    ActionKind Kind{};
  };

  /// \param CGSCCs The SCCs of the call-graph in reverse topological order,
  /// as computed by computeCGSCCs()
  /// \param FollowReturnsPastSeeds Whether summarize() reports exits that are
  /// reachable without a calling context, see
  /// IFDSIDESolverConfig::followReturnsPastSeeds()
  explicit DFIExplodedSupergraph(SCCHolder<FunctionId> CGSCCs,
                                 bool FollowReturnsPastSeeds);

  /// The next step towards the fixpoint, or std::nullopt if there is none.
  /// Prefers callees over callers, and expansion over summarization.
  [[nodiscard]] std::optional<Action> nextAction();

  /// Returns the vertex <Inst, Fact>. New vertices are scheduled for
  /// expansion.
  dfi::VertexId getOrCreateVertex(dfi::InstId Inst, dfi::FactId Fact,
                                  FunctionId Fun, bool IsExit);

  /// Marks Vtx as seed. Returns false, if it was a seed already.
  bool addSeed(dfi::VertexId Vtx);

  /// Marks the vertex Vtx, which must be at a start point, as function entry.
  dfi::EntryId getOrCreateEntry(dfi::VertexId Vtx);

  /// Removes the vertices scheduled for expansion in Fun, sorted by
  /// instruction. The result is valid until the next call of this function.
  [[nodiscard]] llvm::ArrayRef<dfi::VertexId> takeWorklist(FunctionId Fun);

  /// Records the intra-procedural out-edges of Vtx: call beginEdges(Vtx), then
  /// addEdge() for each target, then endEdges(Vtx). Vtx must not be expanded
  /// before. Creating vertices in between is fine.
  void beginEdges(dfi::VertexId Vtx) {
    assert(VtxEdgeBegin[Vtx] == UINT32_MAX &&
           "beginEdges() called before for Vtx!");
    VtxEdgeBegin[Vtx] = uint32_t(Edges.size());
  }
  void addEdge(dfi::VertexId Target) { Edges.push_back(Target); }
  void endEdges(dfi::VertexId Vtx) { VtxEdgeEnd[Vtx] = uint32_t(Edges.size()); }

  /// Adds a summary edge from the call vertex CallVtx to the return-site
  /// vertex RetVtx. Returns false, if the edge was already there.
  bool addSummaryEdge(dfi::VertexId CallVtx, dfi::VertexId RetVtx);

  /// Adds a call edge from CallVtx to Entry. Returns the exits already known
  /// to be reachable from Entry; the solver must apply them to CallVtx. The
  /// result is invalidated by getOrCreateEntry().
  [[nodiscard]] llvm::ArrayRef<dfi::VertexId> addCallEdge(dfi::VertexId CallVtx,
                                                          dfi::EntryId Entry);

  /// Records the unbalanced return from the exit vertex Exit to RetVtx, which
  /// is reachable without a calling context. RetVtx then behaves as if it was
  /// reached from the zero fact at the start of its function.
  void addUnbalancedReturn(dfi::VertexId Exit, dfi::VertexId RetVtx);

  /// Computes the same-level reachability from the entries of Fun to its exit
  /// vertices and calls Handler(CallVtx, Entry, Exit) for each new
  /// (Entry, Exit) pair and each call vertex CallVtx calling Entry.
  ///
  /// With FollowReturnsPastSeeds, also calls UnbalancedHandler(Exit) for each
  /// new exit vertex that is reachable without calling context, i.e., from a
  /// seed without callers or from an unbalanced return. The solver must
  /// return it to all callers of Fun.
  void summarize(
      FunctionId Fun,
      std::invocable<dfi::VertexId, dfi::EntryId, dfi::VertexId> auto Handler,
      std::invocable<dfi::VertexId> auto UnbalancedHandler) {
    indexFunction(Fun);

    for (auto Entry : FunState[Fun].Entries) {
      auto NewExits = updateSummaryExits(Fun, Entry);
      // Handler may add vertices and summary edges, but no entries or callers
      for (size_t I = 0; I != EntryCallers[Entry].size(); ++I) {
        for (auto Exit : NewExits) {
          std::invoke(Handler, EntryCallers[Entry][I], Entry, Exit);
        }
      }
    }

    if (FollowReturnsPastSeeds) {
      // UnbalancedHandler may add vertices, roots and unbalanced returns, but
      // no entries or callers
      for (auto Exit : updateUnbalancedExits(Fun)) {
        std::invoke(UnbalancedHandler, Exit);
      }
    }
  }

  [[nodiscard]] size_t numVertices() const noexcept { return VtxInst.size(); }
  [[nodiscard]] dfi::InstId instOf(dfi::VertexId Vtx) const noexcept {
    return VtxInst[Vtx];
  }
  [[nodiscard]] dfi::FactId factOf(dfi::VertexId Vtx) const noexcept {
    return VtxFact[Vtx];
  }
  [[nodiscard]] FunctionId funOf(dfi::VertexId Vtx) const noexcept {
    return VtxFun[Vtx];
  }
  [[nodiscard]] FunctionId funOf(dfi::EntryId Entry) const noexcept {
    return VtxFun[EntryVertex[Entry]];
  }

  [[nodiscard]] llvm::ArrayRef<dfi::VertexId> seeds() const noexcept {
    return SeedVertices;
  }

  /// The graph of intra-procedural, summary, call, and unbalanced-return edges
  [[nodiscard]] CsrGraph<dfi::VertexId> buildSupergraph() const;

  [[nodiscard]] TypedVector<dfi::VertexId, dfi::InstId>
  takeVertexInsts() noexcept {
    return std::move(VtxInst);
  }
  [[nodiscard]] TypedVector<dfi::VertexId, dfi::FactId>
  takeVertexFacts() noexcept {
    return std::move(VtxFact);
  }

private:
  struct FunctionState {
    TypedVector<dfi::LocalVertexId, dfi::VertexId> Vertices;
    llvm::SmallVector<dfi::VertexId, 0> Worklist;
    /// Entries, seeds, and unbalanced-return targets
    llvm::SmallVector<dfi::VertexId, 0> Roots;
    llvm::SmallVector<dfi::EntryId, 0> Entries;
    llvm::SmallVector<dfi::VertexId, 0> Seeds;
    /// Targets of unbalanced returns into this function
    llvm::SmallVector<dfi::VertexId, 0> UnbalancedRoots;
    llvm::SmallVector<std::pair<dfi::VertexId, dfi::VertexId>, 0> SummaryEdges;
    /// Exit vertices already reported as unbalanced, sorted
    llvm::SmallVector<dfi::VertexId, 0> UnbalancedExits;
    /// Whether the graph changed since the last summarize()
    bool Dirty = false;
  };

  void markDirty(FunctionId Fun);
  bool addRoot(dfi::VertexId Vtx);

  [[nodiscard]] bool hasCallers(dfi::VertexId Vtx) const;
  /// Whether a call reaches the zero fact at a start point of Fun
  [[nodiscard]] bool isZeroEntryCalled(FunctionId Fun) const;

  /// Builds LocalIndex for the vertices of Fun, with exits as targets
  void indexFunction(FunctionId Fun);

  /// Appends the exits reachable from Src according to LocalIndex to NewExits,
  /// unless they are in Known
  void collectNewExits(FunctionId Fun, dfi::VertexId Src,
                       llvm::ArrayRef<dfi::VertexId> Known);

  /// Sorts NewExits, removes duplicates, and merges them into Known
  void mergeNewExits(llvm::SmallVectorImpl<dfi::VertexId> &Known);

  /// Adds the exits reachable from Entry according to LocalIndex to its
  /// summary. Returns the newly added ones. As in the IDESolver, the zero
  /// entry also reaches whatever the unbalanced-return targets reach.
  [[nodiscard]] llvm::ArrayRef<dfi::VertexId>
  updateSummaryExits(FunctionId Fun, dfi::EntryId Entry);

  /// Adds the exits reachable without calling context according to LocalIndex
  /// to the unbalanced exits of Fun. Returns the newly added ones.
  [[nodiscard]] llvm::ArrayRef<dfi::VertexId>
  updateUnbalancedExits(FunctionId Fun);

  [[nodiscard]] llvm::ArrayRef<dfi::VertexId>
  edgesOf(dfi::VertexId Vtx) const noexcept {
    auto Start = VtxEdgeBegin[Vtx];
    auto End = VtxEdgeEnd[Vtx];
    return llvm::ArrayRef(Edges).slice(Start, End - Start);
  }

  SCCHolder<FunctionId> CGSCCs;
  BitSet<SCCId<FunctionId>> PendingSCCs;
  TypedVector<FunctionId, FunctionState> FunState;

  // Per vertex
  llvm::DenseMap<std::pair<dfi::InstId, dfi::FactId>, dfi::VertexId> VertexOf;
  TypedVector<dfi::VertexId, dfi::InstId> VtxInst;
  TypedVector<dfi::VertexId, dfi::FactId> VtxFact;
  TypedVector<dfi::VertexId, FunctionId> VtxFun;
  TypedVector<dfi::VertexId, dfi::LocalVertexId> VtxLocalId;
  /// Intra-procedural out-edges, excluding summary edges:
  /// Edges[VtxEdgeBegin[V] .. VtxEdgeEnd[V])
  TypedVector<dfi::VertexId, uint32_t> VtxEdgeBegin;
  TypedVector<dfi::VertexId, uint32_t> VtxEdgeEnd;
  BitSet<dfi::VertexId> IsExit;
  BitSet<dfi::VertexId> IsRoot;
  llvm::SmallVector<dfi::VertexId, 0> Edges;
  llvm::SmallVector<dfi::VertexId, 0> SeedVertices;

  // Per entry
  llvm::DenseMap<dfi::VertexId, dfi::EntryId> EntryOf;
  TypedVector<dfi::EntryId, dfi::VertexId> EntryVertex;
  TypedVector<dfi::EntryId, llvm::SmallVector<dfi::VertexId, 2>> EntryCallers;
  /// Exit vertices reachable from the entry, sorted
  TypedVector<dfi::EntryId, llvm::SmallVector<dfi::VertexId, 2>> SummaryExits;

  llvm::DenseSet<std::pair<dfi::VertexId, dfi::VertexId>> SummaryEdgeSet;
  llvm::SmallVector<std::pair<dfi::VertexId, dfi::VertexId>, 0> CallEdges;
  llvm::DenseSet<std::pair<dfi::VertexId, dfi::VertexId>> UnbalancedReturnEdges;
  bool FollowReturnsPastSeeds{};

  // Scratch buffers
  llvm::SmallVector<dfi::VertexId, 0> Batch;
  llvm::SmallVector<std::pair<dfi::LocalVertexId, dfi::LocalVertexId>, 0>
      LocalEdges;
  llvm::SmallVector<dfi::LocalVertexId, 0> LocalRoots;
  llvm::SmallVector<dfi::VertexId, 0> NewExits;
  CsrGraph<dfi::LocalVertexId> LocalGraph;
  IntervalReachabilityBuilder<dfi::LocalVertexId> LocalBuilder;
  IntervalReachabilityIndex<dfi::LocalVertexId> LocalIndex;
};

} // namespace psr::detail
