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

#pragma once

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <ostream>
#include <iterator>
#include <phasar/Config/ContainerConfiguration.h>

#include "ContextBase.h"

namespace psr {

/*  N = Node in the CFG
 *  V = Values inside the monotone sets
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
      throw std::runtime_error(
          "initial call std::string length exceeds maximal length K");
    }
  }

  virtual void enterFunction(N src, N dest, MonoSet<V> &In) override {
    if ( k == 0 )
      return;
    if (cs.size() > k - 1) {
      cs.pop_front();
    }
    cs.push_back(src);
  }

  virtual void exitFunction(N src, N dest, MonoSet<V> &In) override {
    if (cs.size() > 0) {
      cs.pop_back();
    }
  }

  virtual bool isUnsure() override {
    // We may be a bit more precise in the future
    if ( cs.size() == k )
      return true;
    else
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
    // That allows an output of a retFlow with an empty callString context
    // to be join with every analysis results at the arrival node.
    return cs < rhs.cs && (cs.size() != 0) && (rhs.cs.size() != 0);
  }

  virtual void print(std::ostream &os) const override {
    std::copy(cs.begin(), --cs.end(), std::ostream_iterator<N>(os, " * "));
    os << cs.back();
  }

  std::size_t size() const { return cs.size(); }
  std::deque<N> getInternalCS() const { return cs; }
};

} // namespace psr
