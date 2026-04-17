/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#pragma once

#include "phasar/Utils/StrongTypeDef.h"

#include <cstdint>

/// Strong ID types for the WPDS automaton and encoding.
///
///   StateId — general automaton state (initial states, witness states, qf).
///   LocId   — control-location ID = compressed dataflow-fact ID.
///             Initial StateIds are numerically identical to LocIds:
///               StateId(to_underlying(L)) is the automaton state for LocId L.
///   SymId   — stack-symbol ID = compressed program-node ID.
///             kEpsilonSym = SymId(UINT32_MAX) is the ε sentinel.
///
/// All three types get llvm::DenseMapInfo specialisations via
/// PHASAR_STRONG_TYPEDEF, so they can be used as DenseMap keys directly.
/// LLVM's generic DenseMapInfo<pair<T,U>> then covers all pair combinations
/// without any hand-written PairDSI.

PHASAR_STRONG_TYPEDEF(psr::wpds, uint32_t, StateId)
PHASAR_STRONG_TYPEDEF(psr::wpds, uint32_t, LocId)
PHASAR_STRONG_TYPEDEF(psr::wpds, uint32_t, SymId)

namespace psr::wpds {

/// Sentinel SymId representing the ε (empty word) stack symbol.
/// Numerically UINT32_MAX — never a valid compressed node ID.
static constexpr SymId kEpsilonSym{~0U};

/// Convert a LocId to the corresponding initial StateId.
/// Valid only for control locations (initial states); the numeric value is
/// shared: StateId(to_underlying(L)) is the automaton state for LocId L.
[[nodiscard]] inline StateId toStateId(LocId L) noexcept {
  return StateId(to_underlying(L));
}

/// Convert an initial StateId back to a LocId.
/// Caller must ensure S is indeed an initial state.
[[nodiscard]] inline LocId toLocId(StateId S) noexcept {
  return LocId(to_underlying(S));
}

} // namespace psr::wpds
