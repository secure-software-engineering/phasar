#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/SparseBitSet.h"
#include "phasar/Utils/TypeTraits.h"

#include <concepts>

namespace psr {

/// Concept for alias-set containers used inside the union-find analyses.
/// The container must support set insertion, membership test, bitwise
/// union/intersection/difference, a \c foreach() visitor, and comparison.
template <typename ASet>
concept IsRawAliasSet = requires(ASet &MutSet, const ASet &ConstSet,
                                 typename ASet::value_type ValId) {
  ASet();
  ASet(ConstSet);
  ASet(std::move(MutSet));
  MutSet = ConstSet;
  MutSet = std::move(MutSet);
  MutSet.insert(ValId);
  { MutSet.tryInsert(ValId) } -> std::convertible_to<bool>;
  { ConstSet.contains(ValId) } -> std::convertible_to<bool>;
  // ConstSet.begin();
  // ConstSet.end();

  /// Iteration must be in ascending order
  ConstSet.foreach (DummyFn<typename ASet::value_type>{});
  MutSet |= ConstSet;
  MutSet &= ConstSet;
  MutSet -= ConstSet;
  { ConstSet - ConstSet } -> std::convertible_to<ASet>;
  { ConstSet == ConstSet } noexcept -> std::convertible_to<bool>;
  { ConstSet != ConstSet } noexcept -> std::convertible_to<bool>;
  { MutSet.tryMergeWith(ConstSet) } -> std::convertible_to<bool>;
  { MutSet.clear() } noexcept;
  { ConstSet.empty() } noexcept -> std::convertible_to<bool>;
  { ConstSet.size() } noexcept -> std::convertible_to<size_t>;
  {
    // Merges the ConstSet into MutSet, as with tryMergeWith, but invokes a
    // callback for each element that was newly inserted.The Diff will be
    // materialized and merged into that out-param
    MutSet.mergeWithDiff(ConstSet, DummyFn<typename ASet::value_type>{}, MutSet)
  } -> std::convertible_to<bool>;
  {
    // Merges the ConstSet into MutSet, as with tryMergeWith, but invokes a
    // callback for each element that was newly inserted.
    MutSet.mergeWithDiff(ConstSet, DummyFn<typename ASet::value_type>{})
  } -> std::convertible_to<bool>;
};

/// For backwards-compatibility only
template <SmallIdType IdT> using RoaringAliasSet = SparseBitSet<IdT>;

/// The default type used for alias/points-to sets
template <SmallIdType IdT> using RawAliasSet = RoaringAliasSet<IdT>;

} // namespace psr
