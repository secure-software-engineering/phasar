/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * AllBottom.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#pragma once

#include <iostream>
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/Utils/Macros.h>
#include <string>
//#include "EdgeIdentity.h"
//#include "AllTop.h"
namespace psr {
template <typename V> class EdgeIdentity;

template <typename V> class AllTop;

template <typename V>
class AllBottom : public EdgeFunction<V>,
                  public std::enable_shared_from_this<AllBottom<V>> {
private:
  const V bottomElement;

public:
  AllBottom(V bottomElement) : bottomElement(bottomElement) {}

  virtual ~AllBottom() = default;

  V computeTarget(V source) override { return bottomElement; }

  virtual std::shared_ptr<EdgeFunction<V>>
  composeWith(std::shared_ptr<EdgeFunction<V>> secondFunction) override {
    if (EdgeIdentity<V> *ei =
            dynamic_cast<EdgeIdentity<V> *>(secondFunction.get()))
      return this->shared_from_this();
    return secondFunction;
  }

  virtual std::shared_ptr<EdgeFunction<V>>
  joinWith(std::shared_ptr<EdgeFunction<V>> otherFunction) override {
    if (otherFunction.get() == this ||
        otherFunction->equalTo(this->shared_from_this()))
      return this->shared_from_this();
    if (AllTop<V> *alltop = dynamic_cast<AllTop<V> *>(otherFunction.get()))
      return this->shared_from_this();
    if (EdgeIdentity<V> *ei =
            dynamic_cast<EdgeIdentity<V> *>(otherFunction.get()))
      return this->shared_from_this();
    // TODO it would be a good idea to make this an exception at some point
    std::cerr << "@ UNEXPECTED EDGE FUNCTION!" << std::endl;
    HEREANDNOW;
    DIE_HARD;
  }

  virtual bool equalTo(std::shared_ptr<EdgeFunction<V>> other) override {
    if (AllBottom<V> *allbottom = dynamic_cast<AllBottom<V> *>(other.get())) {
      return (allbottom->bottomElement == bottomElement);
    }
    return false;
  }

  friend std::ostream &operator<<(std::ostream &os, const AllBottom &allBottom) {
    return os << "allbottom";
  }

  void dump() override { std::cout << "allbottom\n"; }

  std::string toString() override { return "allbottom"; }
};

} // namespace psr
