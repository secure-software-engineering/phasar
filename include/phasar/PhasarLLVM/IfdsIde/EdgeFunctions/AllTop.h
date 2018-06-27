/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * AllTop.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#pragma once

#include <iostream>
#include <memory>

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>

namespace psr {

template <typename V>
class AllTop : public EdgeFunction<V>,
               public std::enable_shared_from_this<AllTop<V>> {
private:
  const V topElement;

public:
  AllTop(V topElement) : topElement(topElement) {}

  virtual ~AllTop() = default;

  virtual V computeTarget(V source) override { return topElement; }

  virtual std::shared_ptr<EdgeFunction<V>>
  composeWith(std::shared_ptr<EdgeFunction<V>> secondFunction) override {
    return this->shared_from_this();
  }

  virtual std::shared_ptr<EdgeFunction<V>>
  joinWith(std::shared_ptr<EdgeFunction<V>> otherFunction) override {
    return otherFunction;
  }

  virtual bool equalTo(std::shared_ptr<EdgeFunction<V>> other) override {
    if (AllTop<V> *alltop = dynamic_cast<AllTop<V> *>(other.get()))
      return (alltop->topElement == topElement);
    return false;
  }

  friend std::ostream &operator<<(std::ostream &os, const AllTop &alltop) {
    return os << "alltop";
  }

  void dump() override { std::cout << "alltop\n"; }

  std::string toString() override { return "alltop"; }
};

} // namespace psr
