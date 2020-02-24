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

#include <map>
#include <memory>
#include <utility>

#include <llvm/Support/ErrorHandling.h>

#include <z3++.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>

namespace psr {

struct z3Less {
  bool operator()(const z3::expr &Lhs, const z3::expr &Rhs) const {
    return Lhs.id() < Rhs.id();
  }
};

template <typename UserL> using VarL = std::map<z3::expr, UserL, z3Less>;

template <typename L> class EdgeFunction;

template <typename UserL>
class VariationalEdgeFunction
    : public EdgeFunction<VarL<UserL>>,
      public std::enable_shared_from_this<VariationalEdgeFunction<UserL>> {
public:
  using user_l_t = UserL;
  using l_t = VarL<UserL>;

private:
  std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>, z3Less>
      UserEdgeFns;

public:
  VariationalEdgeFunction(std::shared_ptr<EdgeFunction<user_l_t>> UserEF,
                          z3::expr Constraint)
      : UserEdgeFns({std::make_pair(Constraint, UserEF)}) {}

  VariationalEdgeFunction(
      std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>, z3Less>
          UserEdgeFns)
      : UserEdgeFns(UserEdgeFns) {}

  ~VariationalEdgeFunction() override = default;

  l_t computeTarget(l_t Source) override {
    std::cout << "Source.size(): " << Source.size() << '\n';
    for (auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
      bool FoundConstraint = false;
      for (auto &[InConstraint, Value] : Source) {
        if (z3::eq(Constraint, InConstraint)) {
          FoundConstraint = true;
          Source[InConstraint] = UserEdgeFn->computeTarget(Value);
        }
      }
      if (!FoundConstraint) {
        // Source[Constraint] = UserEdgeFn->computeTarget(Value);
      }
    }
    return Source;
  }

  std::shared_ptr<EdgeFunction<l_t>>
  composeWith(std::shared_ptr<EdgeFunction<l_t>> secondFunction) override {
    std::cout << "VariationalEdgeFunction::composeWith\n";
    if (auto VEF = dynamic_cast<VariationalEdgeFunction<user_l_t> *>(
            secondFunction.get())) {
      std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>, z3Less>
          ResultUserEdgeFns;
      for (auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
        for (auto &[InConstraint, InUserEdgeFn] : VEF->UserEdgeFns) {
          if (z3::eq(Constraint, InConstraint)) {
            ResultUserEdgeFns[Constraint] =
                UserEdgeFn->composeWith(InUserEdgeFn);
          }
        }
      }
      return std::make_shared<VariationalEdgeFunction>(ResultUserEdgeFns);
    }
    llvm::report_fatal_error("found unexpected edge function");
  }

  std::shared_ptr<EdgeFunction<l_t>>
  joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override {
    std::cout << "VariationalEdgeFunction::joinWith\n";
    if (auto VEF = dynamic_cast<VariationalEdgeFunction<user_l_t> *>(
            otherFunction.get())) {
      std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>, z3Less>
          ResultUserEdgeFns;
      for (auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
        bool FoundConstraint = false;
        for (auto &[InConstraint, InUserEdgeFn] : VEF->UserEdgeFns) {
          if (z3::eq(Constraint, InConstraint)) {
            FoundConstraint = true;
            ResultUserEdgeFns[InConstraint] =
                UserEdgeFn->joinWith(InUserEdgeFn);
          }
        }
        if (!FoundConstraint) {
          ResultUserEdgeFns[Constraint] = UserEdgeFn;
        }
      }
      return std::make_shared<VariationalEdgeFunction<user_l_t>>(ResultUserEdgeFns);
    }
    llvm::report_fatal_error("found unexpected edge function");
  }

  bool equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override {
    std::cout << "VariationalEdgeFunction::equal_to\n";
    if (auto VEF =
            dynamic_cast<VariationalEdgeFunction<user_l_t> *>(other.get())) {
      for (auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
        for (auto &[InConstraint, InUserEdgeFn] : VEF->UserEdgeFns) {
          return true;
        }
      }
    }
    return false;
  }
};

} // namespace psr

#endif
