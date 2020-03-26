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
#include <initializer_list>

#include "boost/bimap.hpp"
#include "boost/bimap/unordered_set_of.hpp"

#include "llvm/ADT/BitVector.h"

namespace psr {

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
  typedef boost::bimap<boost::bimaps::unordered_set_of<T, std::hash<T>>,
                       boost::bimaps::unordered_set_of<size_t>>
      bimap_t;
  inline static bimap_t Position;
  llvm::BitVector Bits;

  template <typename D> class BitVectorSetIterator {
    llvm::BitVector Bits;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = D;
    using difference_type = std::ptrdiff_t;
    using pointer = D *;
    using reference = D &;
    BitVectorSetIterator(D ptr = nullptr) : pos_ptr(ptr) {}
    BitVectorSetIterator(const BitVectorSetIterator<D> &rawIterator) = default;
    ~BitVectorSetIterator() {}

    BitVectorSetIterator<D> &
    operator=(const BitVectorSetIterator<D> &rawIterator) = default;

    BitVectorSetIterator<D> &operator=(D *ptr) {
      pos_ptr = ptr;
      return (*this);
    }

    operator bool() const {
      if (pos_ptr)
        return true;
      return false;
    }

    void setBits(llvm::BitVector B) { Bits = B; }

    bool operator==(const BitVectorSetIterator<D> &rawIterator) const {
      return (pos_ptr == rawIterator.getPtr());
    }

    bool operator!=(const BitVectorSetIterator<D> &rawIterator) const {
      return (pos_ptr != rawIterator.getPtr());
    }

    BitVectorSetIterator<D> &operator+=(const difference_type &movement) {
      for (difference_type i = 0; i < movement; i++)
        pos_ptr++;
      return (*this);
    }

    BitVectorSetIterator<D> &operator++() {
      do {
        if (((pos_ptr->first) >= Bits.size() - 1)) {
          ++pos_ptr;
          break;
        }
        ++pos_ptr;
      } while (!(Bits[pos_ptr->first]));

      return (*this);
    }

    BitVectorSetIterator<D> operator++(int) {
      auto temp(*this);
      ++*this;
      return temp;
    }

    BitVectorSetIterator<D> operator+(const difference_type &movement) {
      auto oldPtr = pos_ptr;
      for (difference_type i = 0; i < movement; i++)
        pos_ptr++;
      auto temp(*this);
      pos_ptr = oldPtr;
      return temp;
    }

    difference_type operator-(const BitVectorSetIterator<D> &rawIterator) {
      return std::distance(rawIterator.getPtr(), this->getPtr());
    }

    // T& operator*(){return pos_ptr->second;}

    const T &operator*() const { return pos_ptr->second; }

    D *operator->() { return pos_ptr; }

    D getPtr() const { return pos_ptr; }

    // const D* getConstPtr()const{return pos_ptr;}

    // T getPos() {return pos_ptr->first;}

    // T getVal() {return pos_ptr->second;}

    llvm::BitVector getBits() { return Bits; }

  protected:
    D pos_ptr;
  };

  typedef typename bimap_t::right_iterator r_iterator;
  typedef typename bimap_t::right_const_iterator rc_iterator;
  typedef BitVectorSetIterator<r_iterator> iterator;
  typedef BitVectorSetIterator<rc_iterator> const_iterator;

public:
  BitVectorSet() = default;

  explicit BitVectorSet(size_t Count) : Bits(Count, false) {}

  BitVectorSet(std::initializer_list<T> Ilist) {
    for (auto &Item : Ilist) {
      insert(Item);
    }
  }

  template <typename InputIt> BitVectorSet(InputIt First, InputIt Last) {
    while (First != Last) {
      insert(*First);
      ++First;
    }
  }

  ~BitVectorSet() = default;

  BitVectorSet<T> setUnion(const BitVectorSet<T> &Other) const {
    size_t size = std::max(Bits.size(), Other.Bits.size());
    BitVectorSet<T> Res(size);
    // temp variable necessary because return type of |= is not const
    llvm::BitVector temp = Bits;
    Res.Bits = temp |= Other.Bits;
    return Res;
  }

  BitVectorSet<T> setIntersect(const BitVectorSet<T> &Other) const {
    size_t size = std::max(Bits.size(), Other.Bits.size());
    BitVectorSet<T> Res(size);
    // temp variable necessary because return type of &= is not const
    llvm::BitVector temp = Bits;
    Res.Bits = temp &= Other.Bits;
    return Res;
  }

  bool includes(const BitVectorSet<T> &Other) const {
    // check if Other contains 1's at positions where this does not
    // Other is longer
    if (Bits.size() < Other.Bits.size()) {
      size_t idx = 0;
      for (; idx < Bits.size(); ++idx) {
        if (Other.Bits[idx] && !Bits[idx]) {
          return false;
        }
      }
      // Check if Other's additional bits are non-zero
      for (; idx < Other.Bits.size(); ++idx) {
        if (Other.Bits[idx]) {
          return false;
        }
      }
      // additional zeros are fine
      return true;
    } else {
      // this is longer or they have the same length
      // check if Other contains 1's at positions where this does not
      for (size_t idx = 0; idx < Other.Bits.size(); ++idx) {
        if (Other.Bits[idx] && !Bits[idx]) {
          return false;
        }
      }
      return true;
    }
  }

  void insert(const T &Data) {
    auto Search = Position.left.find(Data);
    // Data already known
    if (Search != Position.left.end()) {
      if (Bits.size() <= Search->second) {
        Bits.resize(Search->second + 1);
      }
      Bits[Search->second] = true;
    } else {
      // Data unknown
      size_t Idx = Position.left.size();
      Position.left.insert(std::make_pair(Data, Position.left.size()));
      if (Bits.size() <= Position.left.size()) {
        Bits.resize(Position.left.size());
      }
      Bits[Idx] = true;
    }
  }

  void insert(const BitVectorSet<T> &Other) {
    if (Other.Bits.size() > Bits.size()) {
      Bits.resize(Other.Bits.size());
    }
    for (size_t idx = 0; idx < Other.Bits.size(); ++idx) {
      Bits[idx] = (Bits[idx] || Other.Bits[idx]);
    }
  }

  template <typename InputIt> void insert(InputIt First, InputIt Last) {
    while (First != Last) {
      insert(*First);
      ++First;
    }
  }

  void erase(const T &Data) noexcept {
    auto Search = Position.left.find(Data);
    if (Search != Position.left.end()) {
      if (!(Bits.size() < Search->second - 1)) {
        Bits[Search->second] = false;
      }
    }
  }

  void clear() noexcept { Bits.reset(); }

  bool empty() const noexcept { return Bits.none(); }

  void reserve(size_t NewCap) { Bits.reserve(NewCap); }

  bool find(const T &Data) const noexcept { return count(Data); }

  size_t count(const T &Data) const noexcept {
    auto Search = Position.left.find(Data);
    if (Search != Position.left.end()) {
      if (Bits.size() > Search->second) {
        return Bits[Search->second];
      }
    }
    return 0;
  }

  size_t size() const noexcept { return Bits.count(); }

  friend bool operator==(const BitVectorSet &Lhs, const BitVectorSet &Rhs) {
    return Lhs.Bits == Rhs.Bits;
  }

  friend bool operator!=(const BitVectorSet &Lhs, const BitVectorSet &Rhs) {
    return !(Lhs == Rhs);
  }

  friend bool operator<(const BitVectorSet &Lhs, const BitVectorSet &Rhs) {
    return Lhs.Bits.count() < Rhs.Bits.count();
  }

  friend std::ostream &operator<<(std::ostream &OS, const BitVectorSet &B) {
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

  iterator begin() {
    int index = Bits.find_first();
    if (index == -1)
      index = Bits.size();
    iterator ret = Position.right.find(index);
    ret.setBits(Bits);
    return ret;
  }

  iterator end() {
    iterator ret = Position.right.find(Bits.size());
    ret.setBits(Bits);
    return ret;
  }

  const_iterator begin() const {
    int index = Bits.find_first();
    if (index == -1)
      index = Bits.size();
    const_iterator ret = (rc_iterator)Position.right.find(index);
    ret.setBits(Bits);
    return ret;
  }

  const_iterator end() const {
    const_iterator ret = (rc_iterator)Position.right.find(Bits.size());
    ret.setBits(Bits);
    return ret;
  }
};

} // namespace psr

#endif
