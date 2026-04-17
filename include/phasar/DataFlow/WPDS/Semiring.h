/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#pragma once

#include <concepts>

namespace psr {
namespace wpds {

/// Concept for a bounded idempotent semiring as defined in Definition 5 of
/// "Weighted Pushdown Systems and their Application to Interprocedural
/// Dataflow Analysis" by Reps, Schwoon, Jha, Melski (SciCP 58, 2005).
///
/// A bounded idempotent semiring is a quintuple (D, ⊕, ⊗, 0, 1) where:
///   - ⊕ (combine): commutative, associative, idempotent; 0 is the neutral
///     element. Induces a partial order: a ⊑ b iff a ⊕ b = a.
///   - ⊗ (extend): associative (not necessarily commutative); 1 is the
///     neutral element; 0 is an annihilator.
///   - ⊗ distributes over ⊕.
///   - The partial order induced by ⊕ has no infinite descending chains.
///
/// In the interprocedural dataflow setting:
///   - Elements represent dataflow transfer functions or summaries.
///   - extend (⊗) models sequential composition of transfer functions along a
///     path (left-to-right in the paper: f(r) ⊗ g means f applied first).
///   - combine (⊕) models the meet of information from multiple paths.
///   - zero() is the top element (⊕-identity, least precise result).
///   - one() is the identity transfer function (⊗-identity).
template <typename W>
concept BoundedIdempotentSemiring = requires(const W &A, const W &B) {
  /// The zero element: identity for combine (⊕), annihilator for extend (⊗).
  { W::zero() } -> std::convertible_to<W>;
  /// The one element: identity for extend (⊗).
  { W::one() } -> std::convertible_to<W>;
  /// The combine operation (⊕): commutative, idempotent meet.
  /// Returns the "better" (lower under ⊑) of two values.
  { A.combine(B) } -> std::convertible_to<W>;
  /// The extend operation (⊗): sequential composition. Not necessarily
  /// commutative. The weight of path [r1, r2] is f(r1) ⊗ f(r2).
  { A.extend(B) } -> std::convertible_to<W>;
  /// Equality test for fixpoint detection.
  { A == B } -> std::convertible_to<bool>;
};

} // namespace wpds
} // namespace psr
