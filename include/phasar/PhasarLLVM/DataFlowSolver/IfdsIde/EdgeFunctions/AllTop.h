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

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"

namespace psr {

template <typename L>
class AllTop : public EdgeFunction<L>,
               public std::enable_shared_from_this<AllTop<L>> {
public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

private:
  const L topElement;

public:
  AllTop(L topElement) : topElement(topElement) {}

  ~AllTop() override = default;

  L computeTarget(L source) override { return topElement; }

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType secondFunction) override {
    return this->shared_from_this();
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType otherFunction) override {
    return otherFunction;
  }

  bool equal_to(EdgeFunctionPtrType other) const override {
    if (auto *alltop = dynamic_cast<AllTop<L> *>(other.get())) {
      return (alltop->topElement == topElement);
    }
    return false;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "AllTop";
  }
};

} // namespace psr

#endif
