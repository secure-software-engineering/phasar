/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and Richard Leer
 *****************************************************************************/

#ifndef PHASAR_UTILS_BITVECTORSET_H_
#define PHASAR_UTILS_BITVECTORSET_H_

#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/raw_ostream.h"

#include "boost/bimap.hpp"
#include "boost/bimap/unordered_set_of.hpp"

#include <algorithm>
#include <cassert>
#include <initializer_list>

namespace psr {
namespace internal {
inline bool isLess(const llvm::BitVector &Lhs, const llvm::BitVector &Rhs) {
  unsigned LhsBits = Lhs.size();
  unsigned RhsBits = Rhs.size();

  if (LhsBits > RhsBits) {
    if (Lhs.find_first_in(RhsBits, LhsBits) != -1) {
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
} // namespace internal

/**
 * BitVectorSet implements a set that requires minimal space. Elements are
 * kept in a static map and the set itself only stores a vector of bits which
 * indicate whether elements are contained in the set.
 *
 * @brief Implements a set that requires minimal space.
 */
template <typename T> class BitVectorSet {
private:
  // Using boost::hash<T> causes ambiguity for hash_value():
  //  -<llvm/ADT/Hashing.h>
  //  -<boost/functional/hash/extensions.hpp>
  //  -<boost/graph/adjacency_list.hpp>
  using bimap_t = boost::bimap<boost::bimaps::unordered_set_of<T, std::hash<T>>,
                               boost::bimaps::unordered_set_of<size_t>>;
  inline static bimap_t Position; // NOLINT
  llvm::BitVector Bits;

  template <typename D> class BitVectorSetIterator {
    llvm::BitVector Bits;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = D;
    using difference_type = std::ptrdiff_t;
    using pointer = D *;
    using reference = D &;
    BitVectorSetIterator(D Ptr = nullptr) : PosPtr(Ptr) {}

    BitVectorSetIterator<D> &operator=(D *Ptr) {
      PosPtr = Ptr;
      return *this;
    }

    void setBits(const llvm::BitVector &OtherBits) { Bits = OtherBits; }

    bool operator==(const BitVectorSetIterator<D> &OtherIterator) const {
      return PosPtr == OtherIterator.getPtr();
    }

    bool operator!=(const BitVectorSetIterator<D> &OtherIterator) const {
      return !(*this == OtherIterator);
    }

    BitVectorSetIterator<D> &operator+=(const difference_type &Movement) {
      for (difference_type I = 0; I < Movement; I++) {
        PosPtr++;
      }
      return *this;
    }

    BitVectorSetIterator<D> &operator++() {
      do {
        int NextIdx = Bits.find_next(PosPtr->first);

        if (NextIdx <= static_cast<int>(PosPtr->first)) {
          PosPtr = Position.right.find(Bits.size());
          break;
        }

        PosPtr = Position.right.find(NextIdx);

        assert(PosPtr->first < Bits.size() &&
               "pos_ptr->first index into BitVector out of range");
      } while (!Bits[PosPtr->first]);

      return *this;
    }

    BitVectorSetIterator<D> operator++(int) {
      auto Temp(*this);
      ++*this;
      return Temp;
    }

    BitVectorSetIterator<D> operator+(const difference_type &Movement) {
      auto OldPtr = PosPtr;
      for (difference_type I = 0; I < Movement; I++) {
        PosPtr++;
      }
      auto Temp(*this);
      PosPtr = OldPtr;
      return Temp;
    }

    difference_type operator-(const BitVectorSetIterator<D> &OtherIterator) {
      return std::distance(OtherIterator.getPtr(), this->getPtr());
    }

    // T& operator*(){return pos_ptr->second;}

    const T &operator*() const { return PosPtr->second; }

    D *operator->() { return PosPtr; }

    [[nodiscard]] D getPtr() const { return PosPtr; }

    // const D* getConstPtr()const{return pos_ptr;}

    // T getPos() {return pos_ptr->first;}

    // T getVal() {return pos_ptr->second;}

    [[nodiscard]] llvm::BitVector getBits() const { return Bits; }

  private:
    D PosPtr;
  };

public:
  using iterator = BitVectorSetIterator<typename bimap_t::right_iterator>;
  using const_iterator =
      BitVectorSetIterator<typename bimap_t::right_const_iterator>;
  using value_type = T;

  BitVectorSet() = default;

  explicit BitVectorSet(size_t Count) : Bits(Count, false) {}

  BitVectorSet(std::initializer_list<T> IList) {
    insert(IList.begin(), IList.end());
  }

  template <typename InputIt> BitVectorSet(InputIt First, InputIt Last) {
    insert(First, Last);
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
    auto Search = Position.left.find(Data);
    // Data already known
    if (Search != Position.left.end()) {
      if (Bits.size() <= Search->second) {
        Bits.resize(Search->second + 1);
      }
      Bits.set(Search->second);
    } else {
      // Data unknown
      size_t Idx = Position.left.size();
      Position.left.insert(std::make_pair(Data, Position.left.size()));
      if (Bits.size() <= Position.left.size()) {
        Bits.resize(Position.left.size());
      }
      Bits.set(Idx);
    }
  }

  void insert(const BitVectorSet<T> &Other) { Bits |= Other.Bits; }

  template <typename InputIt> void insert(InputIt First, InputIt Last) {
    while (First != Last) {
      insert(*First);
      ++First;
    }
  }

  void erase(const T &Data) noexcept {
    auto Search = Position.left.find(Data);
    if (Search != Position.left.end()) {
      if (Bits.size() > Search->second) {
        Bits.reset(Search->second);
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
    auto Search = Position.left.find(Data);
    if (Search != Position.left.end()) {
      if (Bits.size() > Search->second) {
        return Bits[Search->second];
      }
    }
    return 0;
  }

  [[nodiscard]] size_t size() const noexcept { return Bits.count(); }

  friend bool operator==(const BitVectorSet &Lhs, const BitVectorSet &Rhs) {
    bool LeftEmpty = Lhs.empty();
    bool RightEmpty = Rhs.empty();
    if (LeftEmpty || RightEmpty) {
      return LeftEmpty == RightEmpty;
    }
    // Check, whether Lhs and Rhs actually have the same bits set and not
    // whether their internal representation is exactly identitcal
    auto LhsWords = Lhs.Bits.getData();
    auto RhsWords = Rhs.Bits.getData();
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
    auto Words = BV.Bits.getData();
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
    size_t Idx = 0;
    for (auto &Position : B.Position.left) {
      if (Position.second < B.Bits.size() && B.Bits[Position.second]) {
        ++Idx;
        OS << Position.first;
        if (Idx < B.size()) {
          OS << ", ";
        }
      }
    }
    OS << '>';
    return OS;
  }

  [[nodiscard]] iterator begin() {
    int Index = Bits.find_first();
    if (Index == -1) {
      Index = Bits.size();
    }
    iterator BeginIter(Position.right.find(Index));
    BeginIter.setBits(Bits);
    return BeginIter;
  }

  [[nodiscard]] iterator end() {
    iterator EndIter(Position.right.find(Bits.size()));
    EndIter.setBits(Bits);
    return EndIter;
  }

  [[nodiscard]] const_iterator begin() const {
    int Index = Bits.find_first();
    if (Index == -1) {
      Index = Bits.size();
    }
    const_iterator BeginIter(Position.right.find(Index));
    BeginIter.setBits(Bits);
    return BeginIter;
  }

  [[nodiscard]] const_iterator end() const {
    const_iterator EndIter(Position.right.find(Bits.size()));
    EndIter.setBits(Bits);
    return EndIter;
  }
};

} // namespace psr

namespace std {
template <typename T> struct hash<psr::BitVectorSet<T>> {
  size_t operator()(const psr::BitVectorSet<T> &BVS) noexcept {
    return hash_value(BVS);
  }
};
} // namespace std

#endif
