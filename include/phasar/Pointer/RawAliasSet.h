#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/SparseBitVector.h"

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
  ConstSet.foreach (DummyFn<typename ASet::value_type>{});
  MutSet |= ConstSet;
  MutSet &= ConstSet;
  MutSet -= ConstSet;
  { ConstSet == ConstSet } noexcept -> std::convertible_to<bool>;
  { ConstSet != ConstSet } noexcept -> std::convertible_to<bool>;
  { MutSet.tryMergeWith(ConstSet) } -> std::convertible_to<bool>;
  { MutSet.clear() } noexcept;
  { ConstSet.empty() } noexcept -> std::convertible_to<bool>;
  { ConstSet.size() } noexcept -> std::convertible_to<size_t>;
};

/// Sparse bit-set used to represent alias sets in union-find analyses.
///
/// Currently backed by \c llvm::SparseBitVector for compact storage when ids
/// are scattered in a large range and dense storage when ids are clustered.
/// Satisfies \c IsRawAliasSet.
///
/// \tparam IdT Integer-like id type (e.g., \c ValueId).
template <SmallIdType IdT> class RawAliasSet {
public:
  using value_type = IdT;

  RawAliasSet() = default;

  void insert(IdT Id) { Bits.set(uint32_t(Id)); }

  [[nodiscard]] bool tryInsert(IdT Id) {
    return Bits.test_and_set(uint32_t(Id));
  }

  [[nodiscard]] bool contains(IdT Id) const { return Bits.test(uint32_t(Id)); }

  LLVM_ATTRIBUTE_ALWAYS_INLINE void foreach (
      std::invocable<IdT> auto Handler) const {
    for (auto Bit : Bits) {
      std::invoke(Handler, IdT(Bit));
    }
  }

  void operator|=(const RawAliasSet &Other) { Bits |= Other.Bits; }
  void operator&=(const RawAliasSet &Other) { Bits &= Other.Bits; }
  void operator-=(const RawAliasSet &Other) {
    Bits.intersectWithComplement(Other.Bits);
  }

  [[nodiscard]] bool empty() const noexcept { return Bits.empty(); }
  [[nodiscard]] size_t size() const noexcept { return Bits.count(); }

  void clear() noexcept { Bits.clear(); }

  [[nodiscard]] auto begin() const noexcept { return Bits.begin(); }
  [[nodiscard]] auto end() const noexcept { return Bits.end(); }

  [[nodiscard]] bool tryMergeWith(const RawAliasSet &Other) {
    return Bits |= Other.Bits;
  }

  void erase(IdT Id) { Bits.reset(uint32_t(Id)); }

  [[nodiscard]] bool operator==(const RawAliasSet &Other) const noexcept {
    return Bits == Other.Bits;
  }

private:
  llvm::SparseBitVector<> Bits;
  // TODO: roaring::Roaring Bits;
};
} // namespace psr
