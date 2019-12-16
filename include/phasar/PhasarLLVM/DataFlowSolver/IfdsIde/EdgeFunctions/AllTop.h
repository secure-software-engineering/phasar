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

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>

namespace psr {

template <typename V>
class AllTop : public EdgeFunction<V>{
private:
  const V topElement;

public:
  AllTop(V topElement) : topElement(topElement) {}

  ~AllTop() override = default;

  V computeTarget(V source) override { return topElement; }

  EdgeFunction<V> *composeWith(EdgeFunction<V> *secondFunction) override {
    return this;
  }

  EdgeFunction<V> *joinWith(EdgeFunction<V> *otherFunction) override {
    return otherFunction;
  }

  bool equal_to(EdgeFunction<V> *other) const override {
    if (AllTop<V> *alltop = dynamic_cast<AllTop<V> *>(other))
      return (alltop->topElement == topElement);
    return false;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "AllTop";
  }
};

} // namespace psr

#endif
