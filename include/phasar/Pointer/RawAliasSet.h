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
#include "llvm/ADT/SparseBitVector.h"

#include "roaring/roaring.hh"

#include <concepts>
#include <type_traits>

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

/// Sparse bit-set used to represent alias sets in union-find analyses.
///
/// Currently backed by \c llvm::SparseBitVector for compact storage when ids
/// are scattered in a large range and dense storage when ids are clustered.
/// Satisfies \c IsRawAliasSet.
///
/// \tparam IdT Integer-like id type (e.g., \c ValueId).
template <SmallIdType IdT> class LLVMRawAliasSet {
public:
  using value_type = IdT;

  LLVMRawAliasSet() = default;

  void insert(IdT Id) { Bits.set(uint32_t(Id)); }

  [[nodiscard]] bool tryInsert(IdT Id) {
    return Bits.test_and_set(uint32_t(Id));
  }

  [[nodiscard]] bool contains(IdT Id) const { return Bits.test(uint32_t(Id)); }

  template <std::invocable<IdT> HandlerFn>
  LLVM_ATTRIBUTE_ALWAYS_INLINE void foreach (HandlerFn Handler) const {
    for (auto Bit : Bits) {
      if constexpr (std::convertible_to<std::invoke_result_t<HandlerFn &, IdT>,
                                        bool>) {
        if (!std::invoke(Handler, IdT(Bit))) {
          break;
        }
      } else {
        std::invoke(Handler, IdT(Bit));
      }
    }
  }

  void operator|=(const LLVMRawAliasSet &Other) { Bits |= Other.Bits; }
  void operator&=(const LLVMRawAliasSet &Other) { Bits &= Other.Bits; }
  void operator-=(const LLVMRawAliasSet &Other) {
    Bits.intersectWithComplement(Other.Bits);
  }

  [[nodiscard]] LLVMRawAliasSet operator-(const LLVMRawAliasSet &Other) const {
    LLVMRawAliasSet Ret;
    Ret.Bits = Bits - Other.Bits;
    return Ret;
  }

  [[nodiscard]] bool empty() const noexcept { return Bits.empty(); }
  [[nodiscard]] size_t size() const noexcept { return Bits.count(); }

  void clear() noexcept { Bits.clear(); }

  [[nodiscard]] auto begin() const noexcept { return Bits.begin(); }
  [[nodiscard]] auto end() const noexcept { return Bits.end(); }

  [[nodiscard]] bool tryMergeWith(const LLVMRawAliasSet &Other) {
    return Bits |= Other.Bits;
  }

  void erase(IdT Id) { Bits.reset(uint32_t(Id)); }

  [[nodiscard]] bool operator==(const LLVMRawAliasSet &Other) const noexcept {
    return Bits == Other.Bits;
  }

  bool mergeWithDiff(const LLVMRawAliasSet &Other,
                     std::invocable<IdT> auto WithNewElem,
                     LLVMRawAliasSet &IntoDiff) {
    return mergeWithDiffImpl(Other, copyOrRef(WithNewElem), &IntoDiff);
  }

  bool mergeWithDiff(const LLVMRawAliasSet &Other,
                     std::invocable<IdT> auto WithNewElem) {
    return mergeWithDiffImpl(Other, copyOrRef(WithNewElem), nullptr);
  }

private:
  bool mergeWithDiffImpl(const LLVMRawAliasSet &Other,
                         std::invocable<IdT> auto WithNewElem,
                         LLVMRawAliasSet *IntoDiff) {
    auto Diff = Other.Bits - Bits;
    if (Diff.empty()) {
      return false;
    }

    Bits |= Diff;
    if (IntoDiff) {
      IntoDiff->Bits |= Diff;
    }
    for (auto Elem : Diff) {
      std::invoke(WithNewElem, IdT(Elem));
    }
    return true;
  }

  llvm::SparseBitVector<> Bits;
};

template <SmallIdType IdT> class RoaringAliasSet {
public:
  using value_type = IdT;

  RoaringAliasSet() = default;

  void insert(IdT Id) { Bits.add(uint32_t(Id)); }

  [[nodiscard]] bool tryInsert(IdT Id) { return Bits.addChecked(uint32_t(Id)); }

  [[nodiscard]] bool contains(IdT Id) const {
    return Bits.contains(uint32_t(Id));
  }

  template <std::invocable<IdT> HandlerFn>
  LLVM_ATTRIBUTE_ALWAYS_INLINE void foreach (HandlerFn Handler) const {
    return Bits.iterate(
        [](uint32_t Id, void *HandlerPtr) {
          auto &Handler = *(HandlerFn *)HandlerPtr;
          if constexpr (std::convertible_to<
                            std::invoke_result_t<HandlerFn &, IdT>, bool>) {
            if (!std::invoke(Handler, IdT(Id))) {
              return false;
            }
          } else {
            std::invoke(Handler, IdT(Id));
          }
          return true;
        },
        &Handler);
  }

  void operator|=(const RoaringAliasSet &Other) { Bits |= Other.Bits; }
  void operator&=(const RoaringAliasSet &Other) { Bits &= Other.Bits; }
  void operator-=(const RoaringAliasSet &Other) { Bits -= Other.Bits; }
  [[nodiscard]] RoaringAliasSet operator-(const RoaringAliasSet &Other) const {
    return Bits - Other.Bits;
  }

  [[nodiscard]] bool empty() const noexcept { return Bits.isEmpty(); }
  [[nodiscard]] size_t size() const noexcept { return Bits.cardinality(); }

  void clear() noexcept { Bits.clear(); }

  [[nodiscard]] auto begin() const noexcept { return Bits.begin(); }
  [[nodiscard]] auto end() const noexcept { return Bits.end(); }

  [[nodiscard]] bool tryMergeWith(const RoaringAliasSet &Other) {
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

  [[nodiscard]] bool operator==(const RoaringAliasSet &Other) const noexcept {
    return Bits == Other.Bits;
  }

  bool mergeWithDiff(const RoaringAliasSet &Other,
                     std::invocable<IdT> auto WithNewElem) {
    constexpr size_t DiffThreshold = 16;
    // operator- is expensive, but it is definitely a lot faster than the
    // foreach loop if UPending is large

    if (Other.size() > DiffThreshold) {
      RoaringAliasSet Diff = Other - *this;
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

  bool mergeWithDiff(const RoaringAliasSet &Other,
                     std::invocable<IdT> auto WithNewElem,
                     RoaringAliasSet &IntoDiff) {
    constexpr size_t DiffThreshold = 16;
    // operator- is expensive, but it is definitely a lot faster than the
    // foreach loop if Other is large

    if (Other.size() > DiffThreshold) {
      RoaringAliasSet Diff = Other - *this;
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
  RoaringAliasSet(roaring::Roaring &&RR) : Bits(std::move(RR)) {}

  roaring::Roaring Bits{};
};

template <SmallIdType IdT> using RawAliasSet = RoaringAliasSet<IdT>;

} // namespace psr
