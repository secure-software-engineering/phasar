/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#pragma once

#include "phasar/DataFlow/WPDS/PAutomaton.h"
#include "phasar/DataFlow/WPDS/Semiring.h"
#include "phasar/DataFlow/WPDS/WPDSIds.h"

#include "llvm/ADT/DenseMap.h"

#include <utility>

namespace psr {
namespace wpds {

/// Result of a WPDS post* analysis.
///
/// Holds the saturated P-automaton A_{post*} produced by Algorithm 3 and the
/// per-(fact, node) values V_{d, γ_n} computed by Algorithm 4 (Fig. 19).
///
/// The automaton is the primary result: it represents the meet-over-all-valid-
/// paths information for the entire program in a compact symbolic form.
/// V_{d, γ_n} is the conventional interprocedural dataflow value at node n
/// for fact d — it is recovered by backwards propagation through A_{post*}.
///
/// Keys are (LocId, SymId) pairs where:
///   LocId = compressed ID of the dataflow fact d (= control location in WPDS)
///   SymId = compressed ID of the program node n  (= stack symbol in WPDS)
///
/// \tparam Weight A type satisfying BoundedIdempotentSemiring.
template <typename Weight>
  requires BoundedIdempotentSemiring<Weight>
class WPDSSolverResults {
public:
  using Key = std::pair<LocId, SymId>;

  // DenseMapInfo<pair<LocId, SymId>> is auto-derived via PHASAR_STRONG_TYPEDEF.
  using NodeValueMap = llvm::DenseMap<Key, Weight>;

  explicit WPDSSolverResults(PAutomaton<Weight> Automaton,
                             NodeValueMap NodeValues)
      : Aut(std::move(Automaton)), NodeValues(std::move(NodeValues)) {}

  // ─── Primary result: the saturated post* automaton ─────────────────────────

  [[nodiscard]] const PAutomaton<Weight> &getAutomaton() const noexcept {
    return Aut;
  }

  // ─── Derived result: per-(fact, node) values from Algorithm 4 ──────────────

  /// Returns V_{d, γ_n}: the meet-over-all-valid-paths value for fact d at
  /// node n, computed by Algorithm 4 (backwards propagation through A_{post*}).
  [[nodiscard]] Weight getNodeValue(LocId Fact, SymId Sym) const noexcept {
    auto It = NodeValues.find({Fact, Sym});
    if (It == NodeValues.end()) {
      return Weight::zero();
    }
    return It->second;
  }

  [[nodiscard]] bool hasNodeValue(LocId Fact, SymId Sym) const noexcept {
    return NodeValues.count({Fact, Sym}) != 0;
  }

  [[nodiscard]] const NodeValueMap &getNodeValueMap() const noexcept {
    return NodeValues;
  }

private:
  PAutomaton<Weight> Aut;
  NodeValueMap NodeValues;
};

} // namespace wpds
} // namespace psr
