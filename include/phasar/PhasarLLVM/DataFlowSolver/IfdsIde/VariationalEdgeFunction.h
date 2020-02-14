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

struct z3Less {
  bool operator()(const z3::expr &Lhs, const z3::expr &Rhs) const {
    return Lhs.id() < Rhs.id();
  }
};

template <typename UserL> using VarL = std::map<z3::expr, UserL, z3Less>;

template <typename UserL>
class VariationalEdgeFunction
    : public EdgeFunction<VarL<UserL>>,
      public std::enable_shared_from_this<VariationalEdgeFunction<UserL>> {
public:
  using user_l_t = UserL;
  using l_t = VarL<UserL>;

private:
  std::shared_ptr<EdgeFunction<UserL>> UserEF;
  z3::expr Constraint;

public:
  VariationalEdgeFunction(std::shared_ptr<EdgeFunction<user_l_t>> UserEF,
                          z3::expr Constraint)
      : UserEF(UserEF), Constraint(Constraint) {}

  ~VariationalEdgeFunction() override = default;

  l_t computeTarget(l_t Source) override {
    l_t Res;
    for (auto &[Constraint, Value] : Source) {
      Res[Constraint] = UserEF->computeTarget(Value);
      std::cout << "computeTarget() " << Value << " --> " << Res[Constraint]
                << '\n';
    }
    return Res;
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
    if (auto VEF =
            dynamic_cast<VariationalEdgeFunction<user_l_t> *>(other.get())) {
      return UserEF->equal_to(VEF->UserEF);
    }
    return false;
  }
};

} // namespace psr

#endif
