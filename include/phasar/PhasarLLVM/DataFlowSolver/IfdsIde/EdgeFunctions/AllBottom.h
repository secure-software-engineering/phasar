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
#include <memory>
#include <ostream>
#include <string>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>

namespace psr {

template <typename L> class EdgeIdentity;
template <typename L> class AllTop;

template <typename L>
class AllBottom : public EdgeFunction<L>,
                  public std::enable_shared_from_this<AllBottom<L>> {
private:
  const L bottomElement;

public:
  AllBottom(L bottomElement) : bottomElement(bottomElement) {}

  ~AllBottom() override = default;

  L computeTarget(L source) override { return bottomElement; }

  std::shared_ptr<EdgeFunction<L>>
  composeWith(std::shared_ptr<EdgeFunction<L>> secondFunction) override {
    if (AllBottom<L> *ab = dynamic_cast<AllBottom<L> *>(secondFunction.get())) {
      return this->shared_from_this();
    }
    if (EdgeIdentity<L> *ei =
            dynamic_cast<EdgeIdentity<L> *>(secondFunction.get())) {
      return this->shared_from_this();
    }
    return secondFunction->composeWith(this->shared_from_this());
  }

  std::shared_ptr<EdgeFunction<L>>
  joinWith(std::shared_ptr<EdgeFunction<L>> otherFunction) override {
    if (otherFunction.get() == this ||
        otherFunction->equal_to(this->shared_from_this()))
      return this->shared_from_this();
    if (AllTop<L> *alltop = dynamic_cast<AllTop<L> *>(otherFunction.get()))
      return this->shared_from_this();
    if (EdgeIdentity<L> *ei =
            dynamic_cast<EdgeIdentity<L> *>(otherFunction.get()))
      return this->shared_from_this();
    return this->shared_from_this();
  }

  bool equal_to(std::shared_ptr<EdgeFunction<L>> other) const override {
    if (AllBottom<L> *allbottom = dynamic_cast<AllBottom<L> *>(other.get())) {
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
