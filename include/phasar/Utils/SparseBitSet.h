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
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/ArrayRef.h"

#include "roaring/roaring.hh"

#include <functional>

namespace psr {

/// Thin wrapper over a Roaring bitmap, providing an API conforming to the
/// IsRawAliasSet concept.
template <SmallIdType IdT> class SparseBitSet {
public:
  using value_type = IdT;

  SparseBitSet() = default;

  void insert(IdT Id) { Bits.add(uint32_t(Id)); }

  [[nodiscard]] bool tryInsert(IdT Id) { return Bits.addChecked(uint32_t(Id)); }

  [[nodiscard]] bool contains(IdT Id) const {
    return Bits.contains(uint32_t(Id));
  }

  template <std::invocable<IdT> HandlerFn>
  LLVM_ATTRIBUTE_ALWAYS_INLINE bool foreach (HandlerFn Handler) const {
    // Unfortunately, Bits.iterate() returns void...
    return roaring::api::roaring_iterate(
        &Bits.roaring,
        [](uint32_t Id, void *HandlerPtr) -> bool {
          auto &Handler = *(HandlerFn *)HandlerPtr;
          return invokeControlFlow(Handler, IdT(Id));
        },
        &Handler);
  }

  void operator|=(const SparseBitSet &Other) { Bits |= Other.Bits; }
  void operator&=(const SparseBitSet &Other) { Bits &= Other.Bits; }
  void operator-=(const SparseBitSet &Other) { Bits -= Other.Bits; }
  [[nodiscard]] SparseBitSet operator-(const SparseBitSet &Other) const {
    return Bits - Other.Bits;
  }

  [[nodiscard]] bool empty() const noexcept { return Bits.isEmpty(); }
  [[nodiscard]] size_t size() const noexcept { return Bits.cardinality(); }

  void clear() noexcept { Bits.clear(); }

  [[nodiscard]] auto begin() const noexcept { return Bits.begin(); }
  [[nodiscard]] auto end() const noexcept { return Bits.end(); }

  [[nodiscard]] bool tryMergeWith(const SparseBitSet &Other) {
    auto OldSz = size();
    Bits |= Other.Bits;
    return size() != OldSz;
  }

  void erase(IdT Id) { Bits.remove(uint32_t(Id)); }

  // Bulk-inserts from a sorted, deduplicated array.
  // Roaring constructs containers in O(N) for sorted input.
  void insertSorted(llvm::ArrayRef<uint32_t> Sorted) {
    Bits.addMany(Sorted.size(), Sorted.data());
  }

  [[nodiscard]] bool operator==(const SparseBitSet &Other) const noexcept {
    return Bits == Other.Bits;
  }

  bool mergeWithDiff(const SparseBitSet &Other,
                     std::invocable<IdT> auto WithNewElem) {
    constexpr size_t DiffThreshold = 16;
    // operator- is expensive, but it is definitely a lot faster than the
    // foreach loop if Other is large

    if (Other.size() > DiffThreshold) {
      SparseBitSet Diff = Other - *this;
      if (Diff.empty()) {
        return false;
      }

      *this |= Diff;

      Diff.foreach (copyOrRef(WithNewElem));
      return true;
    }

    bool Ret = false;
    Other.foreach ([&](IdT Elem) {
      if (tryInsert(Elem)) {
        std::invoke(WithNewElem, Elem);
        Ret = true;
      }
    });
    return Ret;
  }

  bool mergeWithDiff(const SparseBitSet &Other,
                     std::invocable<IdT> auto WithNewElem,
                     SparseBitSet &IntoDiff) {
    constexpr size_t DiffThreshold = 16;
    // operator- is expensive, but it is definitely a lot faster than the
    // foreach loop if Other is large

    if (Other.size() > DiffThreshold) {
      SparseBitSet Diff = Other - *this;
      if (Diff.empty()) {
        return false;
      }

      *this |= Diff;
      IntoDiff |= Diff;

      Diff.foreach (copyOrRef(WithNewElem));
      return true;
    }

    bool Ret = false;
    Other.foreach ([&](IdT Elem) {
      if (tryInsert(Elem)) {
        IntoDiff.insert(Elem);
        std::invoke(WithNewElem, Elem);
        Ret = true;
      }
    });

    return Ret;
  }

private:
  SparseBitSet(roaring::Roaring &&RR) : Bits(std::move(RR)) {}

  roaring::Roaring Bits{};
};
} // namespace psr
