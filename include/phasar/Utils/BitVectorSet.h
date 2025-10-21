/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and Richard Leer
 *****************************************************************************/

#ifndef PHASAR_UTILS_BITVECTORSET_H
#define PHASAR_UTILS_BITVECTORSET_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/Fn.h"

#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <iterator>

namespace psr {
namespace internal {

inline bool isLess(const llvm::BitVector &Lhs, const llvm::BitVector &Rhs) {
  unsigned LhsBits = Lhs.size();
  unsigned RhsBits = Rhs.size();

  if (LhsBits > RhsBits) {
    if (Lhs.find_next(RhsBits) != -1) {
      return false;
    }
  } else if (LhsBits < RhsBits) {
    if (Rhs.find_first_in(LhsBits, RhsBits) != -1) {
      return true;
    }
  }

  // Compare every bit on both sides because Lhs and Rhs either have the same
  // amount of bits or all other upper bits of the larger one are zero.
  for (int I = static_cast<int>(std::min(LhsBits, RhsBits)) - 1; I >= 0; --I) {
    if (LLVM_UNLIKELY(Lhs[I] != Rhs[I])) {
      return Rhs[I];
    }
  }
  return false;
}

inline llvm::ArrayRef<uintptr_t> getWords(const llvm::BitVector &BV,
                                          uintptr_t & /*Store*/) {
  return BV.getData();
}
inline llvm::ArrayRef<uintptr_t> getWords(const llvm::SmallBitVector &BV,
                                          uintptr_t &Store) {
  return BV.getData(Store);
}
} // namespace internal

/// BitVectorSet implements a set that requires minimal space. Elements are
/// kept in a static map and the set itself only stores a vector of bits which
/// indicate whether elements are contained in the set.
///
/// \brief Implements a set that requires minimal space.
/// \attention This data-structure is NOT thread-safe, since it relies on static
/// storage!
///
template <typename T, typename BitVectorTy = llvm::BitVector>
class BitVectorSet {

  static ByConstRef<T> iterTransform(uint32_t Idx) {
    assert(Position.inbounds(Idx));
    return Position[Idx];
  }

  // using IteratorTy =
  //     llvm::mapped_iterator<typename BitVectorTy::const_set_bits_iterator,
  //                           fn_t<iterTransform>>;

  // Unfortunately cannot use llvm::mapped_iterator, since the
  // const_set_bits_iterator does not properly implement the iterator interface
  // (typename iterator_category is missing)
  class IteratorTy {
  public:
    using value_type = T;
    using reference = ByConstRef<T>;
    using pointer = const T *;
    using difference_type = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    constexpr IteratorTy(typename BitVectorTy::const_set_bits_iterator It,
                         fn_t<iterTransform> Txn = {}) noexcept
        : It(std::move(It)), Txn(Txn) {}

    IteratorTy &operator++() {
      ++It;
      return *this;
    }
    IteratorTy operator++(int) {
      auto Cpy = *this;
      ++*this;
      return Cpy;
    }

    [[nodiscard]] reference operator*() const { return Txn(*It); }

    [[nodiscard]] bool operator==(const IteratorTy &Other) const {
      return It == Other.It;
    }
    [[nodiscard]] bool operator!=(const IteratorTy &Other) const {
      return !(*this == Other);
    }

  private:
    typename BitVectorTy::const_set_bits_iterator It;
    [[no_unique_address]] fn_t<iterTransform> Txn;
  };

public:
  using const_iterator = IteratorTy;
  using iterator = const_iterator;
  using value_type = T;

  BitVectorSet() = default;

  explicit BitVectorSet(size_t Count) : Bits(Count, false) {}

  BitVectorSet(std::initializer_list<T> IList) {
    insert(IList.begin(), IList.end());
  }

  template <typename InputIt>
  explicit BitVectorSet(InputIt First, InputIt Last) {
    insert(First, Last);
  }

  [[nodiscard]] static BitVectorSet fromBits(BitVectorTy Bits) {
    BitVectorSet Ret;
    Ret.Bits = std::move(Bits);
    return Ret;
  }

  [[nodiscard]] BitVectorSet<T> setUnion(const BitVectorSet<T> &Other) const {
    const bool ThisSetIsSmaller = Bits.size() < Other.Bits.size();
    BitVectorSet<T> Res = ThisSetIsSmaller ? Other : *this;
    const BitVectorSet &Smaller = ThisSetIsSmaller ? *this : Other;

    Res.Bits |= Smaller.Bits;
    return Res;
  }

  [[nodiscard]] BitVectorSet<T>
  setIntersect(const BitVectorSet<T> &Other) const {
    BitVectorSet Res = Bits.size() > Other.Bits.size() ? Other : *this;
    const BitVectorSet &Larger =
        Bits.size() > Other.Bits.size() ? *this : Other;

    Res.Bits &= Larger.Bits;
    return Res;
  }

  void setIntersectWith(const BitVectorSet<T> &Other) { Bits &= Other.Bits; }

  void setUnionWith(const BitVectorSet<T> &Other) { Bits |= Other.Bits; }

  [[nodiscard]] bool includes(const BitVectorSet<T> &Other) const {
    return !Other.Bits.test(Bits);
  }

  void insert(const T &Data) {
    uint32_t Idx = Position.getOrInsert(Data);

    if (Idx >= Bits.size()) {
      Bits.resize(Idx + 1);
    }

    Bits.set(Idx);
  }

  void insert(const BitVectorSet<T> &Other) { Bits |= Other.Bits; }

  template <typename InputIt> void insert(InputIt First, InputIt Last) {
    while (First != Last) {
      insert(*First);
      ++First;
    }
  }

  void erase(const T &Data) noexcept {
    if (auto Idx = Position.getOrNull(Data)) {
      if (*Idx < Bits.size()) {
        Bits.reset(*Idx);
      }
    }
  }

  void erase(const BitVectorSet<T> &Other) {
    if (this == &Other) {
      clear();
    } else {
      Bits.reset(Other.Bits);
    }
  }

  void clear() noexcept {
    Bits.clear();
    Bits.resize(0);
  }

  [[nodiscard]] bool empty() const noexcept { return Bits.none(); }

  void reserve(size_t NewCap) { Bits.reserve(NewCap); }

  [[nodiscard]] bool find(const T &Data) const noexcept { return count(Data); }

  [[nodiscard]] size_t count(const T &Data) const noexcept {
    if (auto Idx = Position.getOrNull(Data)) {
      return (*Idx < Bits.size() && Bits.test(*Idx)) ? 1 : 0;
    }

    return 0;
  }

  [[nodiscard]] size_t size() const noexcept { return Bits.count(); }

  [[nodiscard]] const BitVectorTy &getBits() const & noexcept { return Bits; }
  [[nodiscard]] BitVectorTy &getBits() & noexcept { return Bits; }
  [[nodiscard]] BitVectorTy &&getBits() && noexcept { return std::move(Bits); }

  friend bool operator==(const BitVectorSet &Lhs, const BitVectorSet &Rhs) {
    bool LeftEmpty = Lhs.empty();
    bool RightEmpty = Rhs.empty();
    if (LeftEmpty || RightEmpty) {
      return LeftEmpty == RightEmpty;
    }
    // Check, whether Lhs and Rhs actually have the same bits set and not
    // whether their internal representation is exactly identitcal

    uintptr_t LStore{}, RStore{};
    auto LhsWords = internal::getWords(Lhs.Bits, LStore);
    auto RhsWords = internal::getWords(Rhs.Bits, RStore);
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

  friend bool operator!=(const BitVectorSet &Lhs, const BitVectorSet &Rhs) {
    return !(Lhs == Rhs);
  }

  friend bool operator<(const BitVectorSet &Lhs, const BitVectorSet &Rhs) {
    return internal::isLess(Lhs.Bits, Rhs.Bits);
  }

  // NOLINTNEXTLINE(readability-identifier-naming) -- needed for ADL
  friend llvm::hash_code hash_value(const BitVectorSet &BV) noexcept {
    if (BV.Bits.empty()) {
      return {};
    }
    uintptr_t Store{};
    auto Words = internal::getWords(BV.Bits, Store);
    size_t Idx = Words.size();
    while (Idx && Words[Idx - 1] == 0) {
      --Idx;
    }
    return llvm::hash_combine_range(Words.begin(),
                                    std::next(Words.begin(), Idx));
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const BitVectorSet &B) {
    OS << '<';
    llvm::interleaveComma(B, OS);
    return OS << '>';
  }

  [[nodiscard]] const_iterator begin() const noexcept {
    return {Bits.set_bits_begin(), {}};
  }
  [[nodiscard]] const_iterator end() const noexcept {
    return {Bits.set_bits_end(), {}};
  }

  [[nodiscard]] iterator begin() noexcept {
    return {Bits.set_bits_begin(), {}};
  }
  [[nodiscard]] iterator end() noexcept { return {Bits.set_bits_end(), {}}; }

  static void clearPosition() noexcept { Position.clear(); }

private:
  inline static Compressor<T> Position;

  BitVectorTy Bits;
};

// Overloads with the other intersectWith functions from Utilities.h
template <typename T>
void intersectWith(BitVectorSet<T> &Dest, const BitVectorSet<T> &Src) {
  Dest.setIntersectWith(Src);
}
} // namespace psr

namespace std {
template <typename T> struct hash<psr::BitVectorSet<T>> {
  size_t operator()(const psr::BitVectorSet<T> &BVS) noexcept {
    return hash_value(BVS);
  }
};
} // namespace std

#endif
