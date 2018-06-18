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

#ifndef SRC_ANALYSIS_MONOTONE_CALLSTRING_H_
#define SRC_ANALYSIS_MONOTONE_CALLSTRING_H_

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <iostream>
#include <iterator>
using namespace std;

namespace psr {

template <typename T, unsigned K, typename U> class CallString {
private:
  deque<T> cs;
  static const unsigned k = K;

public:
  CallString() = default;
  CallString(initializer_list<T> ilist) : cs(ilist) {
    if (ilist.size() > k) {
      throw runtime_error(
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

  void enterFunction(T src, T dest, U &In) {
    push(src);
  }

  void exitFunction(T src, T dest, U &In) {
    pop();
  }

  size_t size() { return cs.size(); }
  deque<T> getInternalCS() const { return cs; }
  friend bool operator==(const CallString &lhs, const CallString &rhs) {
    return lhs.cs == rhs.cs || (lhs.cs.size() == 0) || (rhs.cs.size() == 0);
  }
  friend bool operator!=(const CallString &lhs, const CallString &rhs) {
    return !(lhs == rhs);
  }
  friend bool operator<(const CallString &lhs, const CallString &rhs) {
    // Base : lhs.cs < rhs.cs
    // Addition : (lhs.cs.size() != 0) && (rhs.cs.size() != 0)
    // Enable that every empty call-string context match every context
    return lhs.cs < rhs.cs && (lhs.cs.size() != 0) && (rhs.cs.size() != 0);
  }
  friend ostream &operator<<(ostream &os, const CallString &c) {
    copy(c.cs.begin(), --c.cs.end(), std::ostream_iterator<T>(os, " * "));
    os << c.cs.back();
    return os;
  }
};

} // namespace psr

#endif /* SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_ */
