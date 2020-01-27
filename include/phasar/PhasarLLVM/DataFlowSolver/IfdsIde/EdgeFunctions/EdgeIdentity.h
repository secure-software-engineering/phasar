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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_EDGEIDENTITY_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_EDGEIDENTITY_H_

#include <iostream>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h>
#include <string>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h>

namespace psr {

template <typename V> class EdgeFunction;

template <typename V> class EdgeIdentity : public EdgeFunction<V> {
private:
  EdgeIdentity() = default;

public:
  EdgeIdentity(const EdgeIdentity &ei) = delete;

  EdgeIdentity &operator=(const EdgeIdentity &ei) = delete;

  ~EdgeIdentity() override = default;

  V computeTarget(V source) override { return source; }

  EdgeFunction<V> *composeWith(EdgeFunction<V> *secondFunction) override {
    return secondFunction;
  }

  EdgeFunction<V> *joinWith(EdgeFunction<V> *otherFunction) override {
    if ((otherFunction == this) || otherFunction->equal_to(this))
      return this;
    if (AllBottom<V> *ab = dynamic_cast<AllBottom<V> *>(otherFunction))
      return otherFunction;
    if (AllTop<V> *at = dynamic_cast<AllTop<V> *>(otherFunction))
      return this;
    // do not know how to join; hence ask other function to decide on this
    return otherFunction->joinWith(this);
  }

  bool equal_to(EdgeFunction<V> *other) const override { return this == other; }

  static EdgeIdentity<V> *getInstance() {
    // implement singleton C++11 thread-safe (see Scott Meyers)
    static EdgeIdentity<V> *instance = new EdgeIdentity<V>();
    return instance;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "EdgeId";
  }
};

} // namespace psr

#endif
