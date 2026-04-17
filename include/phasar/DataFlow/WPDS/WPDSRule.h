/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#pragma once

#include <cstdint>

namespace psr {
namespace wpds {

/// The kind of a WPDS rule (Definition 1, |w| ≤ 2).
enum class WPDSRuleKind : uint8_t {
  /// (p, γ) ↪ (p', ε): pop the top stack symbol — models function return.
  Pop,
  /// (p, γ) ↪ (p', γ'): replace top stack symbol — models intraprocedural step
  /// or call-to-return-site bypass.
  Internal,
  /// (p, γ) ↪ (p', γ'γ''): replace top and push another — models a function
  /// call (γ' = callee entry, γ'' = return-site pushed on stack).
  Push,
};

/// A single weighted rule in a Weighted Pushdown System.
///
/// Control locations and stack symbols are both represented as uint32_t IDs
/// (compressed by the caller). For the standard single-location interprocedural
/// encoding all rules share the same FromLoc and ToLoc (= p).
///
/// \tparam Weight A type satisfying BoundedIdempotentSemiring.
template <typename Weight> struct WPDSRule {
  WPDSRuleKind Kind;

  uint32_t FromLoc; ///< Source control location p.
  uint32_t FromSym; ///< Source stack symbol γ.
  uint32_t ToLoc;   ///< Target control location p'.

  /// For Internal/Push: first target stack symbol γ'. Unused for Pop.
  uint32_t ToSym1 = 0;
  /// For Push: second target stack symbol γ''. Unused for Pop/Internal.
  uint32_t ToSym2 = 0;

  /// Semiring weight f(r) assigned to this rule.
  Weight Wt;
};

} // namespace wpds
} // namespace psr
