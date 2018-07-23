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

// #include <phasar/Config/ContainerConfiguration.h>

#include "ContextBase.h"

namespace psr {

/*  N = Node in the CFG
 *  D = Domain of possible value (often a map or set but can not be)
 *  K = Maximum depth of CallString
 */
template <typename N, typename D, unsigned K>
class CallString : public ContextBase<N, D, CallString<N,D,K>> {
public:
  using Node_t    = N;
  using Domain_t  = D;

protected:
  std::deque<Node_t> cs;
  static const unsigned k = K;

public:
  CallString() = default;
  CallString(std::initializer_list<Node_t> ilist) : cs(ilist) {
    if (ilist.size() > k) {
      throw std::runtime_error(
          "initial call std::string length exceeds maximal length K");
    }
  }

  virtual void enterFunction(Node_t src, Node_t dest, const Domain_t &In) override {
    if ( k == 0 )
      return;
    if (cs.size() > k - 1) {
      cs.pop_front();
    }
    cs.push_back(src);
  }

  virtual void exitFunction(Node_t src, Node_t dest, const Domain_t &In) override {
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
    std::copy(cs.begin(), --cs.end(), std::ostream_iterator<Node_t>(os, " * "));
    os << cs.back();
  }

  std::size_t size() const { return cs.size(); }
  std::deque<Node_t> getInternalCS() const { return cs; }
};

} // namespace psr
