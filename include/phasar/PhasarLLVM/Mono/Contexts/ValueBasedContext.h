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

#ifndef SRC_ANALYSIS_MONOTONE_VALUEBASEDCONTEXT_H_
#define SRC_ANALYSIS_MONOTONE_VALUEBASEDCONTEXT_H_

#include <algorithm>
#include <iostream>
#include <iterator>
#include <set>
#include <phasar/PhasarLLVM/Mono/Interfaces/ContextBase.h>
#include <phasar/Config/ContainerConfiguration.h>

namespace psr {

/*  N = Node in the CFG
 *  V = Values inside the monotone sets
 */
template <typename N, typename V>
class ValueBasedContext : public ContextBase<N, V, ValueBasedContext<N,V>> {
protected:
  std::set<V> args;

public:
  ValueBasedContext() = default;

  virtual void enterFunction(N src, N dest, MonoSet<V> &In) override {
    //TODO
  }

  virtual void exitFunction(N src, N dest, MonoSet<V> &In) override {
    //TODO
  }

  virtual bool isTotal() override {
    return true;
  }

  virtual bool isEqual(const ValueBasedContext &rhs) const override {
    //TODO
    return false;
  }

  virtual bool isDifferent(const ValueBasedContext &rhs) const override {
    //TODO
    return !isEqual(rhs);
  }

  virtual bool isLessThan(const ValueBasedContext &rhs) const override {
    //TODO
    return true;
  }

  virtual void print(std::ostream &os) const override {
    //TODO
  }
};

} // namespace psr

#endif /* SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_ */
