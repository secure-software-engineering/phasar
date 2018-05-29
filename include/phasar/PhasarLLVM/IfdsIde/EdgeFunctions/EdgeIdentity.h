/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * EdgeIdentity.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_EDGE_FUNC_EDGEIDENTITY_H_
#define ANALYSIS_IFDS_IDE_EDGE_FUNC_EDGEIDENTITY_H_

#include "../EdgeFunction.h"
#include "AllTop.h"
#include <iostream>
#include <memory>
#include <string>

#include "AllBottom.h" // leads to a circular dependency
// Must be resolved at some point, but for now just forward declare the
// AllBottom<V> class
// template<typename V>
// class AllBottom;

using namespace std;

template <typename V>
class EdgeIdentity : public EdgeFunction<V>,
                     public enable_shared_from_this<EdgeIdentity<V>> {
private:
  EdgeIdentity() = default;

public:
  EdgeIdentity(const EdgeIdentity &ei) = delete;

  EdgeIdentity &operator=(const EdgeIdentity &ei) = delete;

  virtual ~EdgeIdentity() = default;

  virtual V computeTarget(V source) override { return source; }

  virtual shared_ptr<EdgeFunction<V>>
  composeWith(shared_ptr<EdgeFunction<V>> secondFunction) override {
    return secondFunction;
  }

  virtual shared_ptr<EdgeFunction<V>>
  joinWith(shared_ptr<EdgeFunction<V>> otherFunction) override {
    if ((otherFunction.get() == this) ||
        otherFunction->equalTo(this->shared_from_this()))
      return this->shared_from_this();
    if (AllBottom<V> *ab = dynamic_cast<AllBottom<V> *>(otherFunction.get()))
      return otherFunction;
    if (AllTop<V> *at = dynamic_cast<AllTop<V> *>(otherFunction.get()))
      return this->shared_from_this();
    // do not know how to join; hence ask other function to decide on this
    return otherFunction->joinWith(this->shared_from_this());
  }

  virtual bool equalTo(shared_ptr<EdgeFunction<V>> other) override {
    return this == other.get();
  }

  static shared_ptr<EdgeIdentity<V>> getInstance() {
    // implement singleton C++11 thread-safe (see Scott Meyers)
    static shared_ptr<EdgeIdentity<V>> instance(new EdgeIdentity<V>());
    return instance;
  }

  friend ostream &operator<<(ostream &os, const EdgeIdentity &edgeIdentity) {
    return os << "edge identity";
  }

  void dump() override { cout << "edge identity\n"; }

  string toString() override { return "edge identity"; }
};

#endif /* ANALYSIS_IFDS_IDE_EDGE_FUNC_EDGEIDENTITY_HH_ */
