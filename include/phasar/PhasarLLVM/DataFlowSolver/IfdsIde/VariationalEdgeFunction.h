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

namespace psr {

template <typename L> class VariationalEdgeFunction : public EdgeFunction<L> {
private:
  std::shared_ptr<EdgeFunction<L>> UserEF;
  // std::shared_ptr<EdgeFunction<z3::expr>> ConstraintEF;

public:
  VariationalEdgeFunction(std::shared_ptr<EdgeFunction<L>> UserEF,
                          z3::expr ConstraintEF)
      : UserEF(UserEF) {}

  ~VariationalEdgeFunction() override = default;

  L computeTarget(L source) override { return UserEF->computeTarget(source); }

  std::shared_ptr<EdgeFunction<L>>
  composeWith(std::shared_ptr<EdgeFunction<L>> secondFunction) override {
    return UserEF->composeWith(secondFunction);
  }

  std::shared_ptr<EdgeFunction<L>>
  joinWith(std::shared_ptr<EdgeFunction<L>> otherFunction) override {
    return UserEF->joinWith(otherFunction);
  }

  bool equal_to(std::shared_ptr<EdgeFunction<L>> other) const override {
    return UserEF->equal_to(other);
  }
};

} // namespace psr

#endif
