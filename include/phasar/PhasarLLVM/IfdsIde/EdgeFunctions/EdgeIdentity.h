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
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/AllTop.h>
#include <string>

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/AllBottom.h>

namespace psr {

template <typename V> class EdgeFunction;

template <typename V>
class EdgeIdentity : public EdgeFunction<V>,
                     public std::enable_shared_from_this<EdgeIdentity<V>> {
private:
  EdgeIdentity() = default;

public:
  EdgeIdentity(const EdgeIdentity &ei) = delete;

  EdgeIdentity &operator=(const EdgeIdentity &ei) = delete;

  virtual ~EdgeIdentity() = default;

  virtual V computeTarget(V source) override { return source; }

  virtual std::shared_ptr<EdgeFunction<V>>
  composeWith(std::shared_ptr<EdgeFunction<V>> secondFunction) override {
    return secondFunction;
  }

  virtual std::shared_ptr<EdgeFunction<V>>
  joinWith(std::shared_ptr<EdgeFunction<V>> otherFunction) override {
    if ((otherFunction.get() == this) ||
        otherFunction->equal_to(this->shared_from_this()))
      return this->shared_from_this();
    if (AllBottom<V> *ab = dynamic_cast<AllBottom<V> *>(otherFunction.get()))
      return otherFunction;
    if (AllTop<V> *at = dynamic_cast<AllTop<V> *>(otherFunction.get()))
      return this->shared_from_this();
    // do not know how to join; hence ask other function to decide on this
    return otherFunction->joinWith(this->shared_from_this());
  }

  virtual bool equal_to(std::shared_ptr<EdgeFunction<V>> other) const override {
    return this == other.get();
  }

  static std::shared_ptr<EdgeIdentity<V>> getInstance() {
    // implement singleton C++11 thread-safe (see Scott Meyers)
    static std::shared_ptr<EdgeIdentity<V>> instance(new EdgeIdentity<V>());
    return instance;
  }

  virtual void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "EdgeIdentity";
  }
};

} // namespace psr

#endif
