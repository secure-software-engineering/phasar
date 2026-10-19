#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/DFISolverConfig.h"
#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/CsrGraph.h"
#include "phasar/Utils/IntervalReachability.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/StrongTypeDef.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>

/// A node <n, d> of the exploded supergraph, materialized by the DFISolver
PHASAR_STRONG_TYPEDEF(psr::dfi, uint32_t, VertexId);
PHASAR_STRONG_TYPEDEF(psr::dfi, uint32_t, InstId);
/// The zero fact always has id 0
PHASAR_STRONG_TYPEDEF(psr::dfi, uint32_t, FactId, Zero = 0);

namespace psr {

namespace detail {
template <typename N, typename D> struct DFIResultsData {
  Compressor<N, dfi::InstId> Insts;
  Compressor<D, dfi::FactId> Facts;

  TypedVector<dfi::VertexId, dfi::InstId> VtxInst;
  TypedVector<dfi::VertexId, dfi::FactId> VtxFact;

  /// The vertices at instruction I are
  /// VerticesAt[InstOffsets[I] .. InstOffsets[I+1]), sorted by fact
  TypedVector<dfi::InstId, uint32_t> InstOffsets{0};
  llvm::SmallVector<dfi::VertexId, 0> VerticesAt;

  std::optional<IntervalReachabilityIndex<dfi::VertexId>> Index;
  DFIIndexDirection IndexDirection = DFIIndexDirection::Forward;
};

template <typename Derived, typename N, typename D> class DFISolverResultsBase {
public:
  using n_t = N;
  using d_t = D;
  using l_t = BinaryDomain;

  /// BinaryDomain::BOTTOM if Fact holds at Stmt, BinaryDomain::TOP otherwise
  [[nodiscard]] l_t resultAt(ByConstRef<n_t> Stmt, ByConstRef<d_t> Fact) const {
    return findVertex(Stmt, Fact) ? BinaryDomain::BOTTOM : BinaryDomain::TOP;
  }

  [[nodiscard]] std::unordered_map<d_t, l_t>
  resultsAt(ByConstRef<n_t> Stmt, bool StripZero = false) const {
    std::unordered_map<d_t, l_t> Ret;
    forEachFactAt(Stmt, StripZero, [&Ret](ByConstRef<d_t> Fact) {
      Ret.try_emplace(Fact, BinaryDomain::BOTTOM);
    });
    return Ret;
  }

  [[nodiscard]] std::set<d_t> ifdsResultsAt(ByConstRef<n_t> Stmt) const {
    std::set<d_t> Ret;
    forEachFactAt(Stmt, /*StripZero*/ false,
                  [&Ret](ByConstRef<d_t> Fact) { Ret.insert(Fact); });
    return Ret;
  }

  /// Calls Handler for each fact holding at Stmt
  void forEachFactAt(ByConstRef<n_t> Stmt, bool StripZero,
                     std::invocable<ByConstRef<d_t>> auto Handler) const {
    const auto &Data = data();
    for (auto Vtx : verticesAt(Stmt)) {
      auto Fact = Data.VtxFact[Vtx];
      if (StripZero && Fact == dfi::FactId::Zero) {
        continue;
      }
      std::invoke(Handler, Data.Facts[Fact]);
    }
  }

  /// Number of instructions with results
  [[nodiscard]] size_t size() const noexcept { return data().Insts.size(); }

  [[nodiscard]] bool containsNode(ByConstRef<n_t> Stmt) const {
    return data().Insts.getOrNull(Stmt).has_value();
  }

  void foreachResultEntry(
      std::invocable<std::tuple<n_t, d_t, l_t>> auto Handler) const {
    const auto &Data = data();
    for (auto [Inst, Stmt] : Data.Insts.enumerate()) {
      for (auto Vtx : verticesAt(Inst)) {
        std::invoke(Handler,
                    std::make_tuple(Stmt, Data.Facts[Data.VtxFact[Vtx]],
                                    BinaryDomain::BOTTOM));
      }
    }
  }

  void dumpResults(llvm::raw_ostream &OS = llvm::outs()) const {
    const auto &Data = data();
    OS << "\n***************************************************************\n"
       << "*                  Raw DFISolver results                      *\n"
       << "***************************************************************\n";
    for (auto [Inst, Stmt] : Data.Insts.enumerate()) {
      OS << "\nN: " << NToString(Stmt) << '\n';
      for (auto Vtx : verticesAt(Inst)) {
        OS << "\tD: " << DToString(Data.Facts[Data.VtxFact[Vtx]]) << '\n';
      }
    }
  }

  /// The vertex for Fact at Stmt, if Fact holds there
  [[nodiscard]] std::optional<dfi::VertexId>
  findVertex(ByConstRef<n_t> Stmt, ByConstRef<d_t> Fact) const {
    const auto &Data = data();
    auto FactId = Data.Facts.getOrNull(Fact);
    if (!FactId) {
      return std::nullopt;
    }
    auto Vertices = verticesAt(Stmt);
    const auto *It =
        std::lower_bound(Vertices.begin(), Vertices.end(), *FactId,
                         [&Data](dfi::VertexId Vtx, dfi::FactId F) {
                           return Data.VtxFact[Vtx] < F;
                         });
    if (It == Vertices.end() || Data.VtxFact[*It] != *FactId) {
      return std::nullopt;
    }
    return *It;
  }

  [[nodiscard]] llvm::ArrayRef<dfi::VertexId>
  verticesAt(ByConstRef<n_t> Stmt) const {
    auto Inst = data().Insts.getOrNull(Stmt);
    if (!Inst) {
      return {};
    }
    return verticesAt(*Inst);
  }

  [[nodiscard]] llvm::ArrayRef<dfi::VertexId>
  verticesAt(dfi::InstId Inst) const {
    return csrSuccs(data().InstOffsets, data().VerticesAt, Inst);
  }

  [[nodiscard]] n_t instOf(dfi::VertexId Vtx) const {
    return data().Insts[data().VtxInst[Vtx]];
  }
  [[nodiscard]] d_t factOf(dfi::VertexId Vtx) const {
    return data().Facts[data().VtxFact[Vtx]];
  }

private:
  [[nodiscard]] const DFIResultsData<N, D> &data() const noexcept {
    return static_cast<const Derived *>(this)->dataImpl();
  }
};
} // namespace detail

/// Non-owning view into the results of a DFISolver. Conforms to the interface
/// expected by GenericSolverResults.
template <typename N, typename D>
class [[gsl::Pointer]] DFISolverResults
    : public detail::DFISolverResultsBase<DFISolverResults<N, D>, N, D> {
  friend detail::DFISolverResultsBase<DFISolverResults<N, D>, N, D>;

public:
  explicit DFISolverResults(const detail::DFIResultsData<N, D> *Data
                                PSR_LIFETIME_CAPTURE_BY(this)) noexcept
      : Data(Data) {
    assert(Data != nullptr);
  }

private:
  [[nodiscard]] const detail::DFIResultsData<N, D> &dataImpl() const noexcept {
    return *Data;
  }

  const detail::DFIResultsData<N, D> *Data{};
};

/// Owning variant of DFISolverResults
template <typename N, typename D>
class OwningDFISolverResults
    : public detail::DFISolverResultsBase<OwningDFISolverResults<N, D>, N, D> {
  friend detail::DFISolverResultsBase<OwningDFISolverResults<N, D>, N, D>;

public:
  explicit OwningDFISolverResults(
      std::unique_ptr<const detail::DFIResultsData<N, D>> Data) noexcept
      : Data(std::move(Data)) {
    assert(this->Data != nullptr);
  }

  [[nodiscard]] DFISolverResults<N, D> get() const & noexcept {
    return DFISolverResults<N, D>(Data.get());
  }
  DFISolverResults<N, D> get() && = delete;

  [[nodiscard]] operator DFISolverResults<N, D>() const & noexcept {
    return get();
  }
  operator DFISolverResults<N, D>() && = delete;

private:
  [[nodiscard]] const detail::DFIResultsData<N, D> &dataImpl() const noexcept {
    return *Data;
  }

  std::unique_ptr<const detail::DFIResultsData<N, D>> Data;
};

/// Context-sensitive reachability between the exploded-supergraph nodes that
/// the DFISolver has materialized.
///
/// From reaches To iff there is a realizable path from From to To that may
/// descend into callees, but does not return beyond the function of From.
/// Every node reaches itself. The nodes reachable from a node are its
/// descendants, the nodes reaching it are its ancestors; both include the node
/// itself.
///
/// Point queries cost O(log k) in both index directions. Enumerating the
/// descendants (forward index) or ancestors (backward index) is linear in the
/// output; the other enumeration scans all nodes.
template <typename N, typename D> class DFIReachability {
public:
  using n_t = N;
  using d_t = D;

  explicit DFIReachability(const detail::DFIResultsData<N, D> *Data) noexcept
      : Results(Data), Index(&*Data->Index), Direction(Data->IndexDirection) {}

  /// The node <Stmt, Fact>, if it has been materialized, i.e., if Fact holds
  /// at Stmt.
  [[nodiscard]] std::optional<dfi::VertexId> find(ByConstRef<n_t> Stmt,
                                                  ByConstRef<d_t> Fact) const {
    return Results.findVertex(Stmt, Fact);
  }

  [[nodiscard]] bool reaches(dfi::VertexId From,
                             dfi::VertexId To) const noexcept {
    if (Direction == DFIIndexDirection::Backward) {
      return Index->reaches(To, From);
    }
    return Index->reaches(From, To);
  }

  [[nodiscard]] bool reaches(ByConstRef<n_t> FromStmt, ByConstRef<d_t> FromFact,
                             ByConstRef<n_t> ToStmt,
                             ByConstRef<d_t> ToFact) const {
    auto From = find(FromStmt, FromFact);
    auto To = find(ToStmt, ToFact);
    return From && To && reaches(*From, *To);
  }

  /// Calls Handler for each fact at At that is reachable from From
  void
  forEachDescendantFactAt(dfi::VertexId From, ByConstRef<n_t> At,
                          std::invocable<ByConstRef<d_t>> auto Handler) const {
    for (auto To : Results.verticesAt(At)) {
      if (reaches(From, To)) {
        std::invoke(Handler, Results.factOf(To));
      }
    }
  }

  /// Calls Handler for each fact at At that reaches To
  void
  forEachAncestorFactAt(dfi::VertexId To, ByConstRef<n_t> At,
                        std::invocable<ByConstRef<d_t>> auto Handler) const {
    for (auto From : Results.verticesAt(At)) {
      if (reaches(From, To)) {
        std::invoke(Handler, Results.factOf(From));
      }
    }
  }

  /// Calls Handler for each node reachable from From
  void forEachDescendant(
      dfi::VertexId From,
      std::invocable<ByConstRef<n_t>, ByConstRef<d_t>> auto Handler) const {
    forEachRelated(From, DFIIndexDirection::Forward, Handler);
  }

  /// Calls Handler for each node that reaches To
  void forEachAncestor(
      dfi::VertexId To,
      std::invocable<ByConstRef<n_t>, ByConstRef<d_t>> auto Handler) const {
    forEachRelated(To, DFIIndexDirection::Backward, Handler);
  }

  [[nodiscard]] n_t instOf(dfi::VertexId Vtx) const {
    return Results.instOf(Vtx);
  }
  [[nodiscard]] d_t factOf(dfi::VertexId Vtx) const {
    return Results.factOf(Vtx);
  }

  [[nodiscard]] const IntervalReachabilityIndex<dfi::VertexId> &
  index() const noexcept {
    return *Index;
  }

  [[nodiscard]] DFIIndexDirection indexDirection() const noexcept {
    return Direction;
  }

private:
  /// Calls Handler for each node related to Vtx in direction Dir
  void forEachRelated(
      dfi::VertexId Vtx, DFIIndexDirection Dir,
      std::invocable<ByConstRef<n_t>, ByConstRef<d_t>> auto &Handler) const {
    const auto Report = [&](dfi::VertexId Related) {
      std::invoke(Handler, Results.instOf(Related), Results.factOf(Related));
    };

    if (Dir == Direction) {
      Index->forEachReachableTarget(Vtx, Report);
      return;
    }

    // The index points the other way, so the related nodes are not contiguous
    for (auto Other : iota<dfi::VertexId>(Index->numVertices())) {
      if (Index->reaches(Other, Vtx)) {
        Report(Other);
      }
    }
  }

  DFISolverResults<N, D> Results;
  const IntervalReachabilityIndex<dfi::VertexId> *Index{};
  DFIIndexDirection Direction{};
};

} // namespace psr
