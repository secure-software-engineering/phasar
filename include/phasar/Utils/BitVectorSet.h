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

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <map>
#include <vector>

namespace psr {

// TODO: under construction
// BitVectorSet must allow for fast set union/set intersection/insert/lookup
// while requireing minimal space which, in turn, allows for fast copies to be
// performed (propagation along the control-flow graph).

/**
 * BitVectorSet implements a set that only requires minimal space. Elements are
 * kept in a static map and the set itself only stores a vector of bits which
 * indicate whether elements are contained in the set.
 *
 * @brief Implements a set that only requires minimal space.
 */
template <typename T> class BitVectorSet {
private:
  inline static std::map<T, size_t> Position;
  std::vector<bool> Bits;

public:
  BitVectorSet() = default;

  BitVectorSet(std::initializer_list<T> Ilist) {
    for (auto &Item : Ilist) {
      insert(Item);
    }
  }

  BitVectorSet(const BitVectorSet &) = default;

  BitVectorSet(BitVectorSet &&) = default;

  ~BitVectorSet() = default;

  void insert(const T &Data) {
    auto Search = Position.find(Data);
    // Data already known
    if (Search != Position.end()) {
      if (Bits.size() <= Search->second) {
        Bits.resize(Search->second + 1);
      }
      Bits[Search->second] = true;
    } else {
      // Data unknown
      size_t Idx = Position.size();
      Position[Data] = Position.size();
      if (Bits.size() <= Position.size()) {
        Bits.resize(Position.size());
      }
      Bits[Idx] = true;
    }
  }

  void erase(const T &Data) noexcept {
    auto Search = Position.find(Data);
    if (Search != Position.end()) {
      if (!(Bits.size() < Search->second - 1)) {
        Bits[Search->second] = false;
      }
    }
  }

  void clear() noexcept { std::fill(Bits.begin(), Bits.end(), false); }

  bool empty() const noexcept {
    return std::find(Bits.begin(), Bits.end(), true) == Bits.end();
  }

  bool find(const T &Data) const noexcept { return count(Data); }

  size_t count(const T &Data) const noexcept {
    auto Search = Position.find(Data);
    if (Search != Position.end()) {
      if (Bits.size() - 1 >= Search->second) {
        return Bits[Search->second];
      }
    }
    return 0;
  }

  size_t size() const noexcept {
    return std::count_if(Bits.begin(), Bits.end(),
                         [](bool b) { return b == true; });
  }

  friend bool operator==(const BitVectorSet &Lhs, const BitVectorSet &Rhs) {
    if (Lhs.Bits.size() == Rhs.Bits.size()) {
      for (size_t Idx = 0; Idx < Lhs.Bits.size(); ++Idx) {
        if (Lhs.Bits[Idx] != Rhs.Bits[Idx]) {
          return false;
        }
      }
      return true;
    }
    const BitVectorSet &Shorter =
        (Lhs.Bits.size() < Rhs.Bits.size()) ? Lhs : Rhs;
    const BitVectorSet &Longer =
        (Lhs.Bits.size() > Rhs.Bits.size()) ? Lhs : Rhs;
    for (size_t Idx = 0; Idx < Shorter.Bits.size(); ++Idx) {
      if (Lhs.Bits[Idx] != Rhs.Bits[Idx]) {
        return false;
      }
    }
    return std::count_if(std::next(Longer.Bits.begin(), Shorter.Bits.size()),
                         Longer.Bits.end(),
                         [](bool b) { return b == true; }) == 0;
  }

  friend bool operator!=(const BitVectorSet &Lhs, const BitVectorSet &Rhs) {
    return !(Lhs == Rhs);
  }

  friend std::ostream &operator<<(std::ostream &OS, const BitVectorSet &B) {
    OS << '<';
    for (auto &Position : B.Position) {
      if (Position.second < B.Bits.size() && B.Bits[Position.second]) {
        OS << Position.first << ", ";
      }
    }
    OS << '>';
    return OS;
  }
};

} // namespace psr

#endif
