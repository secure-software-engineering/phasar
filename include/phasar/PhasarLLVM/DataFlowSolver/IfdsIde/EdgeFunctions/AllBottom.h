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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_ALLBOTTOM_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_ALLBOTTOM_H_

#include <iostream> // std::cerr
#include <ostream>
#include <string>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>
#include <phasar/Utils/Macros.h>

namespace psr {

template <typename V> class EdgeIdentity;
template <typename V> class AllTop;

template <typename V> class AllBottom : public EdgeFunction<V> {
private:
  const V bottomElement;

public:
  AllBottom(V bottomElement) : bottomElement(bottomElement) {}

  ~AllBottom() override = default;

  V computeTarget(V source) override { return bottomElement; }

  EdgeFunction<V> *composeWith(EdgeFunction<V> *secondFunction) override {
    if (AllBottom<V> *ab = dynamic_cast<AllBottom<V> *>(secondFunction)) {
      return this;
    }
    if (EdgeIdentity<V> *ei = dynamic_cast<EdgeIdentity<V> *>(secondFunction)) {
      return this;
    }
    return secondFunction->composeWith(this);
  }

  EdgeFunction<V> *joinWith(EdgeFunction<V> *otherFunction) override {
    if (otherFunction == this || otherFunction->equal_to(this))
      return this;
    if (AllTop<V> *alltop = dynamic_cast<AllTop<V> *>(otherFunction))
      return this;
    if (EdgeIdentity<V> *ei = dynamic_cast<EdgeIdentity<V> *>(otherFunction))
      return this;
    return this;
  }

  bool equal_to(EdgeFunction<V> *other) const override {
    if (AllBottom<V> *allbottom = dynamic_cast<AllBottom<V> *>(other)) {
      return (allbottom->bottomElement == bottomElement);
    }
    return false;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "AllBottom";
  }
};

} // namespace psr

#endif
