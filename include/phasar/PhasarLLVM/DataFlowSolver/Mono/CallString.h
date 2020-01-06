/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * CallString.h
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_CALLSTRING_H_
#define PHASAR_PHASARLLVM_MONO_CALLSTRING_H_

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <stdexcept>

namespace psr {

template <typename T, unsigned K> class CallString {
private:
  std::deque<T> cs;
  static const unsigned k = K;

public:
  CallString() = default;
  CallString(std::initializer_list<T> ilist) : cs(ilist) {
    if (ilist.size() > k) {
      throw std::runtime_error(
          "initial call string length exceeds maximal length K");
    }
  }
  void push(T s) {
    if (cs.size() > k - 1) {
      cs.pop_front();
    }
    cs.push_back(s);
  }
  T returnSite() {
    if (cs.size() > 0)
      return cs.back();
    return nullptr;
  }
  void pop() {
    if (cs.size() > 0) {
      cs.pop_back();
    }
  }
  size_t size() { return cs.size(); }
  std::deque<T> getInternalCS() const { return cs; }
  friend bool operator==(const CallString &lhs, const CallString &rhs) {
    return lhs.cs == rhs.cs;
  }
  friend bool operator!=(const CallString &lhs, const CallString &rhs) {
    return !(lhs == rhs);
  }
  friend bool operator<(const CallString &lhs, const CallString &rhs) {
    return lhs.cs < rhs.cs;
  }
  friend std::ostream &operator<<(std::ostream &os, const CallString &c) {
    std::copy(c.cs.begin(), --c.cs.end(), std::ostream_iterator<T>(os, " * "));
    os << c.cs.back();
    return os;
  }
};

} // namespace psr

#endif
