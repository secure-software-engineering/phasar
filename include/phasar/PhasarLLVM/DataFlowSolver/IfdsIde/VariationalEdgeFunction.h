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
#include <utility>

#include <z3++.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>

namespace psr {

template <typename L> class VariationalEdgeFunction : public EdgeFunction<L> {
public:
  using l_t = std::pair<L, z3::expr>;

private:
  std::shared_ptr<EdgeFunction<l_t>> UserEF;
  std::shared_ptr<EdgeFunction<z3::expr>> ConstraintEF;

public:
  VariationalEdgeFunction(std::shared_ptr<EdgeFunction<l_t>> UserEF,
                          std::shared_ptr<EdgeFunction<z3::expr>> ConstraintEF)
      : UserEF(UserEF), ConstraintEF(ConstraintEF) {}

  ~VariationalEdgeFunction() override = default;

  l_t computeTarget(l_t source) override {
    return std::make_pair(UserEF->computeTarget(source.first), source.second);
  }

  std::shared_ptr<EdgeFunction<l_t>>
  composeWith(std::shared_ptr<EdgeFunction<l_t>> secondFunction) override {
    std::cout << "VariationalEdgeFunction::composeWith\n";
    if (auto VEF =
            dynamic_cast<VariationalEdgeFunction *>(secondFunction.get())) {
      return std::make_shared<VariationalEdgeFunction<l_t>>(
          UserEF->composeWith(VEF->UserEF),
          ConstraintEF->composeWith(VEF->ConstraintEF));
    }
    return UserEF->composeWith(secondFunction);
  }

  std::shared_ptr<EdgeFunction<l_t>>
  joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override {
    std::cout << "VariationalEdgeFunction::joinWith\n";
    return UserEF->joinWith(otherFunction);
  }

  bool equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override {
    std::cout << "VariationalEdgeFunction::equal_to\n";
    return UserEF->equal_to(other);
  }
};

} // namespace psr

#endif
