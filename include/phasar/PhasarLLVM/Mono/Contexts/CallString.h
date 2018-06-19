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
#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/Mono/Values/ValueBase.h>

#include "ContextBase.h"


namespace psr {

/*  N = Node in the CFG
 *  V = Values inside the monotone sets (must be a sub class of ValueBase)
 *  K = Maximum depth of CallString
 */
template <typename N, typename V, unsigned K>
class CallString : public ContextBase<N, V, CallString<N,V,K>> {
protected:
  std::deque<N> cs;
  static const unsigned k = K;

public:
  CallString() = default;
  CallString(std::initializer_list<N> ilist) : cs(ilist) {
    if (ilist.size() > k) {
      throw runtime_error(
          "initial call string length exceeds maximal length K");
    }
  }

  void push(N s) {
    if (cs.size() > k - 1) {
      cs.pop_front();
    }
    cs.push_back(s);
  }

  N returnSite() {
    if (cs.size() > 0)
      return cs.back();
    return nullptr;
  }

  void pop() {
    if (cs.size() > 0) {
      cs.pop_back();
    }
  }

  virtual void enterFunction(N src, N dest, MonoSet<V> &In) override {
    push(src);
  }

  virtual void exitFunction(N src, N dest, MonoSet<V> &In) override {
    pop();
  }

  virtual bool isTotal() override {
    // We may be a bit more precise in the future
    return false;
  }

  virtual bool isEqual(const CallString &rhs) const override {
    return cs == rhs.cs || (cs.size() == 0) || (rhs.cs.size() == 0);
  }

  virtual bool isDifferent(const CallString &rhs) const override {
    return !isEqual(rhs);
  }

  virtual bool isLessThan(const CallString &rhs) const override {
    // Base : lhs.cs < rhs.cs
    // Addition : (lhs.cs.size() != 0) && (rhs.cs.size() != 0)
    // Enable that every empty call-string context match every context
    return cs < rhs.cs && (cs.size() != 0) && (rhs.cs.size() != 0);
  }

  virtual void print(std::ostream &os) const override {
    std::copy(cs.begin(), --cs.end(), std::ostream_iterator<N>(os, " * "));
    os << cs.back();
  }

  size_t size() const { return cs.size(); }
  std::deque<N> getInternalCS() const { return cs; }
};

} // namespace psr

#endif /* SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_ */
