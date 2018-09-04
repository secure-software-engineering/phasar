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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_ALLTOP_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_ALLTOP_H_

#include <iosfwd>
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

  virtual bool equal_to(std::shared_ptr<EdgeFunction<V>> other) const override {
    if (AllTop<V> *alltop = dynamic_cast<AllTop<V> *>(other.get()))
      return (alltop->topElement == topElement);
    return false;
  }

  virtual void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "AllTop";
  }
};

} // namespace psr

#endif
