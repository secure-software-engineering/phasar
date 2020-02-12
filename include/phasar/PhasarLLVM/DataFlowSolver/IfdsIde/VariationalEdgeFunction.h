/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_VARIATIONALEDGEFUNCTION_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_VARIATIONALEDGEFUNCTION_H_

#include <memory>

#include <z3++.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>

// Dummy for variability-aware edge function: Implementation will be added later

namespace psr {

template <typename L> class VariationalEdgeFunction : public EdgeFunction<L> {
public:
  VariationalEdgeFunction(const std::shared_ptr<EdgeFunction<L>> &userEF,
                          z3::expr cond) {}

  ~VariationalEdgeFunction() override = default;

  // TODO implement
  L computeTarget(L source) override { return source; }

  std::shared_ptr<EdgeFunction<L>>
  composeWith(std::shared_ptr<EdgeFunction<L>> secondFunction) override {
    return secondFunction;
  }

  std::shared_ptr<EdgeFunction<L>>
  joinWith(std::shared_ptr<EdgeFunction<L>> otherFunction) override {
    return otherFunction;
  }

  bool equal_to(std::shared_ptr<EdgeFunction<L>> other) const override {
    return false;
  }
};

} // namespace psr

#endif
