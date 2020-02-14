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
#include <vector>

#include <llvm/Support/ErrorHandling.h>

#include <z3++.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h>

namespace psr {

template <typename UserL>
class VariationalEdgeFunction
    : public EdgeFunction<std::vector<std::pair<UserL, z3::expr>>>,
      public std::enable_shared_from_this<VariationalEdgeFunction<UserL>> {
public:
  using l_t = std::vector<std::pair<UserL, z3::expr>>;
  using user_l_t = UserL;

private:
  std::shared_ptr<EdgeFunction<UserL>> UserEF;
  z3::expr Constraint;

public:
  VariationalEdgeFunction(std::shared_ptr<EdgeFunction<user_l_t>> UserEF,
                          z3::expr Constraint)
      : UserEF(UserEF), Constraint(Constraint) {}

  ~VariationalEdgeFunction() override = default;

  l_t computeTarget(l_t Source) override {
    // std::cout << "Source.size(): " << Source.size() << '\n';
    if (Source.size() == 1) {
      auto P = std::make_pair(UserEF->computeTarget(Source[0].first),
                              Source[0].second);
      return {P};
    }
    return Source;
  }

  std::shared_ptr<EdgeFunction<l_t>>
  composeWith(std::shared_ptr<EdgeFunction<l_t>> secondFunction) override {
    std::cout << "VariationalEdgeFunction::composeWith\n";
    if (auto VEF = dynamic_cast<VariationalEdgeFunction<user_l_t> *>(
            secondFunction.get())) {
      return std::make_shared<VariationalEdgeFunction<user_l_t>>(
          UserEF->composeWith(VEF->UserEF), Constraint);
    }
    llvm::report_fatal_error("found unexpected edge function");
  }

  std::shared_ptr<EdgeFunction<l_t>>
  joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override {
    std::cout << "VariationalEdgeFunction::joinWith\n";
    if (auto VEF = dynamic_cast<VariationalEdgeFunction<user_l_t> *>(
            otherFunction.get())) {
      return std::make_shared<VariationalEdgeFunction<user_l_t>>(
          UserEF->joinWith(VEF->UserEF), Constraint);
    }
    llvm::report_fatal_error("found unexpected edge function");
  }

  bool equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override {
    std::cout << "VariationalEdgeFunction::equal_to\n";
    if (auto VEF = dynamic_cast<VariationalEdgeFunction<user_l_t> *>(
            other.get())) {
      return UserEF->equal_to(VEF->UserEF);
    }
    return false;
  }
};

} // namespace psr

#endif
