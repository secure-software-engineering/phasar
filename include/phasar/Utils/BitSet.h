/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and other
 *****************************************************************************/

#ifndef PHASAR_UTILS_BITSET_H
#define PHASAR_UTILS_BITSET_H

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallBitVector.h"

#include <bit>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>

namespace psr {

/// \brief A set-type that can compactly store sets of sequential integer-like
/// types.
///
/// Use this type for sequential (unsigned) integers and ids that can convert
/// from and to uint32_t.
///
/// \tparam IdT The type of elements to store in this set. Must be losslessly
/// convertible from and to uint32_t.
/// \tparam BitVectorTy The underlying bit-vector to use. Must be either
/// llvm::BitVector or llvm::SmallBitVector.
template <SmallIdType IdT, typename BitVectorTy = llvm::BitVector>
class BitSet {
  static llvm::ArrayRef<uintptr_t> getWords(const llvm::BitVector &BV,
                                            uintptr_t & /*Store*/) {
    // getData() accesses Bits[0] unconditionally; guard against empty vector.
    if (BV.empty()) {
      return {};
    }
    return BV.getData();
  }
  static llvm::ArrayRef<uintptr_t> getWords(const llvm::SmallBitVector &BV,
                                            uintptr_t &Store) {
    if (BV.empty()) {
      return {};
    }
    return BV.getData(Store);
  }

public:
  /// Wraps BitVectorTy::const_set_bits_iterator, as LLVM's bitset iterators
  /// unfortunately do not conform to the named requirement of an iterator
  class Iterator {
  public:
    using value_type = IdT;
    using reference = IdT;
    using pointer = const IdT *;
    using difference_type = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    Iterator(typename BitVectorTy::const_set_bits_iterator It) noexcept
        : It(It) {}

    Iterator &operator++() noexcept {
      ++It;
      return *this;
    }
    Iterator operator++(int) noexcept {
      auto Ret = *this;
      ++*this;
      return Ret;
    }
    reference operator*() const noexcept { return IdT(*It); }

    bool operator==(const Iterator &Other) const noexcept {
      return It == Other.It;
    }
    bool operator!=(const Iterator &Other) const noexcept {
      return !(*this == Other);
    }

  private:
    typename BitVectorTy::const_set_bits_iterator It;
  };

  using iterator = Iterator;
  using value_type = IdT;

  BitSet() noexcept = default;
  explicit BitSet(size_t InitialCapacity) : Bits(InitialCapacity) {}
  explicit BitSet(size_t InitialCapacity, bool InitialValue)
      : Bits(InitialCapacity, InitialValue) {}

  void reserve(size_t Cap) {
    if (Bits.size() < Cap) {
      Bits.resize(Cap);
    }
  }

  [[nodiscard]] bool contains(IdT Id) const noexcept {
    auto Index = uint32_t(Id);
    return Bits.size() > Index && Bits.test(Index);
  }

  void insert(IdT Id) {
    auto Index = uint32_t(Id);
    if (Bits.size() <= Index) {
      Bits.resize(Index + 1);
    }

    Bits.set(Index);
  }

  /// Same as insert(), but returns, whether the set was changed.
  [[nodiscard]] bool tryInsert(IdT Id) {
    auto Index = uint32_t(Id);
    if (Bits.size() <= Index) {
      Bits.resize(Index + 1);
    }

    bool Ret = !Bits.test(Index);
    Bits.set(Index);
    return Ret;
  }

  void erase(IdT Id) noexcept {
    if (Bits.size() > size_t(Id)) {
      Bits.reset(uint32_t(Id));
    }
  }
  /// Same as erase(), but returns, whether the set was changed.
  [[nodiscard]] bool tryErase(IdT Id) noexcept {
    if (contains(Id)) {
      return Bits.reset(uint32_t(Id)), true;
    }

    return false;
  }

  void mergeWith(const BitSet &Other) { Bits |= Other.Bits; }

  /// Same as mergeWith(), but returns, whether the set was changed.
  bool tryMergeWith(const BitSet &Other) {
    /// TODO: Make this more efficient
    return isSupersetOf(Other) ? false : (mergeWith(Other), true);
  }

  void clear() noexcept { Bits.reset(); }

  [[nodiscard]] friend bool operator==(const BitSet &Lhs,
                                       const BitSet &Rhs) noexcept {
    // Check, whether Lhs and Rhs actually have the same bits set and not
    // whether their internal representation is exactly identitcal
    uintptr_t LhsStore{};
    uintptr_t RhsStore{};

    auto LhsWords = getWords(Lhs.Bits, LhsStore);
    auto RhsWords = getWords(Rhs.Bits, RhsStore);
    if (LhsWords.size() == RhsWords.size()) {
      return LhsWords == RhsWords;
    }
    auto MinSize = std::min(LhsWords.size(), RhsWords.size());
    if (LhsWords.slice(0, MinSize) != RhsWords.slice(0, MinSize)) {
      return false;
    }
    auto Rest = (LhsWords.size() > RhsWords.size() ? LhsWords : RhsWords)
                    .slice(MinSize);
    return std::all_of(Rest.begin(), Rest.end(),
                       [](auto Word) { return Word == 0; });
  }

  [[nodiscard]] friend bool operator!=(const BitSet &Lhs,
                                       const BitSet &Rhs) noexcept {
    return !(Lhs == Rhs);
  }

  [[nodiscard]] bool any() const noexcept { return Bits.any(); }

  [[nodiscard]] iterator begin() const noexcept {
    return Bits.set_bits_begin();
  }
  [[nodiscard]] iterator end() const noexcept { return Bits.set_bits_end(); }

  /// Returns the smallest element in the set, or std::nullopt if empty.
  [[nodiscard]] std::optional<IdT> findFirst() const noexcept {
    const int First = Bits.find_first();
    return First >= 0 ? std::optional<IdT>(IdT(First)) : std::nullopt;
  }

  /// Returns the smallest element greater than \p Key, or std::nullopt if
  /// none exists.
  [[nodiscard]] std::optional<IdT> findNext(IdT Key) const noexcept {
    const int Next = Bits.find_next(int(uint32_t(Key)));
    return Next >= 0 ? std::optional<IdT>(IdT(Next)) : std::nullopt;
  }

  /// Calls the given handler function for each set bit in the bitset.
  ///
  /// This is likely faster than using iterators.
  template <std::invocable<IdT> HandlerFn>
  void foreach (HandlerFn Handler) const
      noexcept(std::is_nothrow_invocable_v<HandlerFn &, IdT>) {
    uintptr_t Store{};
    auto Words = getWords(Bits, Store);
    uint32_t Offset = 0;
    for (auto W : Words) {
      while (W) {
        auto Curr = std::countr_zero(W) + Offset;
        W &= W - 1;
        std::invoke(Handler, IdT(Curr));
      }

      Offset += sizeof(W) * CHAR_BIT;
    }
  }

  /// Same as mergeWith()
  void operator|=(const BitSet &Other) { Bits |= Other.Bits; }
  void operator-=(const BitSet &Other) { Bits.reset(Other.Bits); }

  [[nodiscard]] BitSet operator-(const BitSet &Other) const {
    // TODO: keep allocation small by looping from the end and truncating all
    // words that result in all-zero
    auto Ret = *this;
    Ret -= Other;
    return Ret;
  }

  /// Same as mergeWith(), but returns *this to allow a fluent interface.
  BitSet &insertAllOf(const BitSet &Other) {
    Bits |= Other.Bits;
    return *this;
  }
  /// Same as operator-=, but returns *this to allow a fluent interface.
  BitSet &eraseAllOf(const BitSet &Other) {
    Bits.reset(Other.Bits);
    return *this;
  }

  [[nodiscard]] bool isSubsetOf(const BitSet &Of) const {
    uintptr_t Buf = 0;
    uintptr_t OfBuf = 0;

    auto Words = getWords(Bits, Buf);
    auto OfWords = getWords(Of.Bits, OfBuf);
    if (Words.size() > OfWords.size()) {
      if (llvm::any_of(Words.drop_front(OfWords.size()),
                       [](uintptr_t W) { return W != 0; })) {
        return false;
      }
    }

    for (auto [W, OfW] : llvm::zip(Words, OfWords)) {
      if ((W & ~OfW) != 0) {
        return false;
      }
    }

    return true;
  }

  [[nodiscard]] bool isSupersetOf(const BitSet &Of) const {
    return Of.isSubsetOf(*this);
  }

  /// The number of bits available. This operation is O(1)
  [[nodiscard]] size_t capacity() const noexcept { return Bits.size(); }
  /// The number of bits set to 1. In contrast to most other containers, this
  /// operation is linear in O(capacity())
  [[nodiscard]] size_t size() const noexcept { return Bits.count(); }
  /// Whether this set contains no elements. In contrast to most other
  /// containers, this operation is linear in O(capacity())
  [[nodiscard]] bool empty() const noexcept { return Bits.none(); }

private:
  BitVectorTy Bits;
};
} // namespace psr

#endif
