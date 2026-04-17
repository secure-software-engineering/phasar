/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#pragma once

#include "phasar/DataFlow/WPDS/WPDSIds.h"

namespace psr::wpds {

/// The kind of a WPDS rule (Definition 1, |w| ≤ 2).
enum class WPDSRuleKind : uint8_t {
  /// (p, γ) ↪ (p', ε): pop the top stack symbol — models function return.
  Pop,
  /// (p, γ) ↪ (p', γ'): replace top stack symbol — models intraprocedural
  /// step or call-to-return-site bypass.
  Internal,
  /// (p, γ) ↪ (p', γ'γ''): replace top and push another — models a function
  /// call (γ' = callee entry, γ'' = return-site pushed on stack).
  Push,
};

/// A single weighted rule in a Weighted Pushdown System.
///
/// \tparam Weight A type satisfying BoundedIdempotentSemiring.
template <typename Weight> struct WPDSRule {
  WPDSRuleKind Kind;

  LocId FromLoc; ///< Source control location p.
  SymId FromSym; ///< Source stack symbol γ.
  LocId ToLoc;   ///< Target control location p'.

  /// For Internal/Push: first target stack symbol γ'. Unused for Pop.
  SymId ToSym1{};
  /// For Push: second target stack symbol γ''. Unused for Pop/Internal.
  SymId ToSym2{};

  Weight Wt;
};

} // namespace psr::wpds
