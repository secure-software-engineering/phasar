/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Nicolas Bellec and others
 *****************************************************************************/

/*
 * ValueBasedContext.h
 *
 *  Created on: 19.06.2018
 *      Author: nicolas
 */

// Generic implementation, could be far more precise if the analysis
// takes into account the variables as we can reduce the number of
// args copied to match exactly the function arguments + the global variables
// used by the function (+ static variables ?)

#pragma once

#include <algorithm>
#include <ostream>
#include <set>

#include <phasar/Config/ContainerConfiguration.h>

#include "ContextBase.h"


namespace psr {

/*  N = Node in the CFG
 *  Value = Values inside the monotone sets
 */
template <typename N, typename Value>
class ValueBasedContext : public ContextBase<N, Value, ValueBasedContext<N,Value>> {
protected:
  MonoSet<Value> args;
  MonoSet<Value> prev_context;

public:
  ValueBasedContext() = default;

  virtual void enterFunction(N src, N dest, MonoSet<Value> &In) override {
    prev_context.swap(args); // O(1)
    args = In; // O(|In|)
  }

  virtual void exitFunction(N src, N dest, MonoSet<Value> &In) override {
    args.swap(prev_context); // O(1)
    prev_context.clear(); // O(|prev_context|)
  }

  virtual bool isUnsure() override {
    return false;
  }

  virtual bool isEqual(const ValueBasedContext &rhs) const override {
    if (rhs.args.size() != args.size())
      return false;

    return std::equal(args.cbegin(), args.cend(), rhs.args.cbegin());
  }

  virtual bool isDifferent(const ValueBasedContext &rhs) const override {
    return !isEqual(rhs);
  }

  virtual bool isLessThan(const ValueBasedContext &rhs) const override {
    if ( args.size() < rhs.args.size() )
      return true;

    auto lhs_it = args.cbegin();
    const auto lhs_end = args.cend();

    auto rhs_it = rhs.args.cbegin();
    const auto rhs_end = rhs.args.cend();

    while ( lhs_it != lhs_end && rhs_it != rhs_end ) {
      if ( *lhs_it < *rhs_it )
        return true;
      if ( *lhs_it > *rhs_it )
        return false;
      ++lhs_it;
      ++rhs_it;
    }

    return false;
  }

  virtual void print(std::ostream &os) const override {
    //TODO
  }
};

} // namespace psr
