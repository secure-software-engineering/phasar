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

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"

#include <cstdint>
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
/// Keys are (factId, symId) pairs where:
///   factId = compressed ID of the dataflow fact d (= control location in WPDS)
///   symId  = compressed ID of the program node n  (= stack symbol in WPDS)
///
/// \tparam Weight A type satisfying BoundedIdempotentSemiring.
template <typename Weight>
  requires BoundedIdempotentSemiring<Weight>
class WPDSSolverResults {
public:
  using StateId = typename PAutomaton<Weight>::StateId;
  using Key = std::pair<uint32_t, uint32_t>; ///< (factId, symId)

  struct PairDSI {
    static Key getEmptyKey() noexcept { return {UINT32_MAX, UINT32_MAX}; }
    static Key getTombstoneKey() noexcept {
      return {UINT32_MAX - 1, UINT32_MAX - 1};
    }
    static unsigned getHashValue(Key V) noexcept {
      return llvm::hash_combine(V.first, V.second);
    }
    static bool isEqual(Key A, Key B) noexcept { return A == B; }
  };

  using NodeValueMap = llvm::DenseMap<Key, Weight, PairDSI>;

  explicit WPDSSolverResults(PAutomaton<Weight> Automaton,
                             NodeValueMap NodeValues)
      : Aut(std::move(Automaton)), NodeValues(std::move(NodeValues)) {}

  // ─── Primary result: the saturated post* automaton ─────────────────────────

  /// Returns the saturated P-automaton A_{post*}.
  [[nodiscard]] const PAutomaton<Weight> &getAutomaton() const noexcept {
    return Aut;
  }

  // ─── Derived result: per-(fact, node) values from Algorithm 4 ──────────────

  /// Returns V_{d, γ_n} where FactId is the compressed ID of fact d and SymId
  /// is the compressed ID of node n.
  ///
  /// This is the meet-over-all-valid-paths value for fact d at node n,
  /// computed by Algorithm 4 (backwards propagation through A_{post*}).
  [[nodiscard]] Weight getNodeValue(uint32_t FactId,
                                    uint32_t SymId) const noexcept {
    auto It = NodeValues.find({FactId, SymId});
    if (It == NodeValues.end())
      return Weight::zero();
    return It->second;
  }

  /// Returns true iff a V_{d, γ_n} value has been recorded for (FactId, SymId).
  [[nodiscard]] bool hasNodeValue(uint32_t FactId,
                                  uint32_t SymId) const noexcept {
    return NodeValues.count({FactId, SymId}) != 0;
  }

  /// Access the raw node-value map (for iteration or export).
  /// Keys are (factId, symId) pairs.
  [[nodiscard]] const NodeValueMap &getNodeValueMap() const noexcept {
    return NodeValues;
  }

private:
  PAutomaton<Weight> Aut;
  NodeValueMap NodeValues;
};

} // namespace wpds
} // namespace psr
