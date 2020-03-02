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
#include <functional>
#include <initializer_list>
#include <iostream>
#include <set>
#include <vector>

#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>

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
  std::vector<bool> Bits;

  template <typename D> class MyIterator {
    std::vector<bool> Bits;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = D;
    using difference_type = std::ptrdiff_t;
    using pointer = D *;
    using reference = D &;
    MyIterator(D ptr = NULL) : pos_ptr(ptr) {}
    MyIterator(const MyIterator<D> &rawIterator) = default;
    ~MyIterator() {}

    MyIterator<D> &operator=(const MyIterator<D> &rawIterator) = default;
    MyIterator<D> &operator=(D *ptr) {
      pos_ptr = ptr;
      return (*this);
    }
    operator bool() const {
      if (pos_ptr)
        return true;
      return false;
    }

    void setBits(std::vector<bool> B) { Bits = B; }

    bool operator==(const MyIterator<D> &rawIterator) const {
      return (pos_ptr == rawIterator.getPtr());
    }
    bool operator!=(const MyIterator<D> &rawIterator) const {
      return (pos_ptr != rawIterator.getPtr());
    }

    MyIterator<D> &operator+=(const difference_type &movement) {
      for (difference_type i = 0; i < movement; i++)
        pos_ptr++;
      return (*this);
    }
    MyIterator<D> &operator-=(const difference_type &movement) {
      for (difference_type i = 0; i < movement; i++)
        pos_ptr--;
      return (*this);
    }
    MyIterator<D> &operator++() {
      do {
        ++pos_ptr;
      } while (!((pos_ptr->first) >= Bits.size()) && !Bits[pos_ptr->first]);
      return (*this);
    }
    MyIterator<D> &operator--() {
      do {
        --pos_ptr;
      } while (!Bits[pos_ptr->first]);
      return (*this);
    }

    MyIterator<D> operator++(int) {
      auto temp(*this);
      ++pos_ptr;
      return temp;
    }
    MyIterator<D> operator--(int) {
      auto temp(*this);
      --pos_ptr;
      return temp;
    }

    // ERROR: USAGE GIVING AMBIGUITY ERROR
    MyIterator<D> operator+(const difference_type &movement) {
      auto oldPtr = pos_ptr;
      for (difference_type i = 0; i < movement; i++)
        pos_ptr++;
      auto temp(*this);
      pos_ptr = oldPtr;
      return temp;
    }
    MyIterator<D> operator-(const difference_type &movement) {
      auto oldPtr = pos_ptr;
      for (difference_type i = 0; i < movement; i++)
        pos_ptr--;
      auto temp(*this);
      pos_ptr = oldPtr;
      return temp;
    }

    difference_type operator-(const MyIterator<D> &rawIterator) {
      return std::distance(rawIterator.getPtr(), this->getPtr());
    }

    // T& operator*(){return pos_ptr->second;}
    const T &operator*() const { return pos_ptr->second; }
    D *operator->() { return pos_ptr; } // don't know what it should do

    D getPtr() const { return pos_ptr; }
    // const D* getConstPtr()const{return pos_ptr;}

  protected:
    D pos_ptr;
  };

  typedef typename bimap_t::right_iterator r_iterator;
  typedef MyIterator<r_iterator> iterator;
  typedef MyIterator<const r_iterator> const_iterator;

public:
  iterator begin() {
    iterator ret = Position.right.find(
        std::distance(Bits.begin(), std::find(Bits.begin(), Bits.end(), true)));
    ret.setBits(Bits);
    return ret;
  }
  iterator end() {
    // end at "last occurence of 1" + 1
    // FIXME: Bits.size() does not necessarily contain a "1"
    iterator ret = Position.right.find(Bits.size());
    ret.setBits(Bits);
    return ret;
  }
  // const_iterator begin() const {
  //   iterator ret = Position.right.find(std::distance(Bits.begin(),
  //   std::find(Bits.begin(), Bits.end(), true))); ret.setBits(Bits); return
  //   ret;
  // }

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
    const std::vector<bool> *Shorter, *Longer;
    if (Bits.size() < Other.Bits.size()) {
      Shorter = &Bits;
      Longer = &Other.Bits;
    } else {
      Shorter = &Other.Bits;
      Longer = &Bits;
    }
    BitVectorSet<T> Res(Longer->size());
    size_t idx = 0;
    for (; idx < Shorter->size(); ++idx) {
      Res.Bits[idx] = ((*Shorter)[idx] || (*Longer)[idx]);
    }
    for (; idx < Longer->size(); ++idx) {
      Res.Bits[idx] = (*Longer)[idx];
    }
    return Res;
  }

  BitVectorSet<T> setIntersect(const BitVectorSet<T> &Other) const {
    const std::vector<bool> *Shorter, *Longer;
    if (Bits.size() < Other.Bits.size()) {
      Shorter = &Bits;
      Longer = &Other.Bits;
    } else {
      Shorter = &Other.Bits;
      Longer = &Bits;
    }
    BitVectorSet<T> Res(Shorter->size());
    for (size_t idx = 0; idx < Shorter->size(); ++idx) {
      Res.Bits[idx] = ((*Shorter)[idx] && (*Longer)[idx]);
    }
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

  void clear() noexcept { std::fill(Bits.begin(), Bits.end(), false); }

  bool empty() const noexcept {
    return std::find(Bits.begin(), Bits.end(), true) == Bits.end();
  }

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

  friend bool operator<(const BitVectorSet &Lhs, const BitVectorSet &Rhs) {
    return Lhs.Bits < Rhs.Bits;
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

  std::set<T> getAsSet() const {
    std::set<T> Elements;
    for (size_t idx = 0; idx < Bits.size(); ++idx) {
      if (Bits[idx]) {
        auto e = Position.right.find(idx);
        if (e != Position.right.end()) {
          Elements.insert(e->second);
        }
      }
    }
    return Elements;
  }

  // void printtoomuch() { // trying the behaviour
  //   r_iterator rit = Position.right.find(0);
  //   for (int i = 0; i < 10; i++) {
  //     std::cout << rit->first << " " << rit->second << " -- ";
  //     rit++;
  //   }
  //   // for(int i = 10; i>0;i--){
  //   // rit--;
  //   // NOT WORKING: PROBABLY THERE IS NO DECREMENT OPERATOR IN BIMAP
  //   // ITERATOR std::cout << rit->first<<" " << rit->second << " -- ";
  //   // }
  // }
};

} // namespace psr

#endif
