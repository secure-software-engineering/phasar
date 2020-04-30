/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_VAREDGEFUNCTIONS_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_VAREDGEFUNCTIONS_H_

#include <map>
#include <memory>
#include <utility>

#include <z3++.h>

#include "llvm/Support/ErrorHandling.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"

namespace psr {

struct Z3Less {
  bool operator()(const z3::expr &Lhs, const z3::expr &Rhs) const {
    return Lhs.id() < Rhs.id();
  }
};

template <typename L> using VarL = std::map<z3::expr, L, Z3Less>;

template <typename L> bool ContainsZ3Expr(const VarL<L> &M, const z3::expr &E) {
  bool FoundKey = false;
  for (auto &[Key, Value] : M) {
    if (z3::eq(Key, E)) {
      FoundKey = true;
      break;
    }
  }
  return FoundKey;
}

class ConstraintEdgeFunction
    : public EdgeFunction<z3::expr>,
      public std::enable_shared_from_this<ConstraintEdgeFunction> {
private:
  z3::expr Constraint;

public:
  ConstraintEdgeFunction(z3::expr Constraint) : Constraint(Constraint) {}

  ~ConstraintEdgeFunction() override = default;

  z3::expr computeTarget(z3::expr Source) override { return Source; }

  std::shared_ptr<EdgeFunction<z3::expr>>
  composeWith(std::shared_ptr<EdgeFunction<z3::expr>> secondFunction) override {
    std::cout << "ConstraintEdgeFunction::composeWith\n";
    llvm::report_fatal_error("found unexpected edge function");
  }

  std::shared_ptr<EdgeFunction<z3::expr>>
  joinWith(std::shared_ptr<EdgeFunction<z3::expr>> otherFunction) override {
    std::cout << "ConstraintEdgeFunction::joinWith\n";
    llvm::report_fatal_error("found unexpected edge function");
  }

  bool equal_to(std::shared_ptr<EdgeFunction<z3::expr>> other) const override {
    std::cout << "ConstraintEdgeFunction::equal_to\n";
    return false;
  }
};

template <typename L>
class VarEdgeFunction
    : public EdgeFunction<VarL<L>>,
      public std::enable_shared_from_this<VarEdgeFunction<L>> {
public:
  using user_l_t = L;
  using l_t = VarL<L>;

private:
  std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>, Z3Less>
      UserEdgeFns;

public:
  VarEdgeFunction(std::shared_ptr<EdgeFunction<user_l_t>> UserEdgeFn,
                  z3::expr Constraint)
      : UserEdgeFns({std::make_pair(Constraint, UserEdgeFn)}) {
    std::cout << "VAREdgeFunction: UserEdgeFn, Constraint: "
              << Constraint.to_string() << '\n';
  }

  VarEdgeFunction(
      std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>, Z3Less>
          UserEdgeFns)
      : UserEdgeFns(UserEdgeFns) {
    std::cout << "VAREdgeFunction: UserEdgeFns\n";
  }

  ~VarEdgeFunction() override = default;

  l_t computeTarget(l_t Source) override {
    std::cout << "IN --> Source.size(): " << Source.size()
              << ", UserEdgeFns.size(): " << UserEdgeFns.size() << '\n';
    // for (auto &[C, V] : Source) {
    //   std::cout << "<" << C.to_string() << " , " << V << ">, ";
    // }
    // std::cout << '\n';
    auto ResSource = Source;
    for (auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
      std::cout << "ContainsZ3Expr( " << Constraint.to_string() << " ) --> "
                << ContainsZ3Expr(Source, Constraint) << '\n';
      if (ContainsZ3Expr(Source, Constraint)) {
        ResSource[Constraint] = UserEdgeFn->computeTarget(Source[Constraint]);
      } else {
        ResSource[Constraint] = UserEdgeFn->computeTarget(0);
      }
    }
    std::cout << "OUT --> ResSource.size(): " << ResSource.size() << '\n';
    return ResSource;
  }

  std::shared_ptr<EdgeFunction<l_t>>
  composeWith(std::shared_ptr<EdgeFunction<l_t>> secondFunction) override {
    std::cout << "VarEdgeFunction::composeWith\n";
    if (auto VEF =
            dynamic_cast<VarEdgeFunction<user_l_t> *>(secondFunction.get())) {
      std::cout << "UserEdgeFns.size(): " << UserEdgeFns.size()
                << " --- VEF->UserEdgeFns.size(): " << VEF->UserEdgeFns.size()
                << '\n';
      // We need to compose the constraints as well as the user edge functions.
      // One of the maps will contain one entry only that needs to be composed
      // with the other map (which may contains multiple entries).
      auto &OneEntryMap =
          (VEF->UserEdgeFns.size() == 1) ? VEF->UserEdgeFns : UserEdgeFns;
      auto &MulEntryMap =
          (VEF->UserEdgeFns.size() != 1) ? VEF->UserEdgeFns : UserEdgeFns;
      std::cout << "OneEntryMap.size(): " << OneEntryMap.size()
                << " --- MulEntryMap.size(): " << MulEntryMap.size() << '\n';
      auto UserEdgeFn = *OneEntryMap.begin();
      std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>, Z3Less>
          ResultUserEdgeFns;
      for (auto &[C, EF] : MulEntryMap) {
        // compose constraints and edge functions
        auto ComposedConstraint = C && UserEdgeFn.first;
        ResultUserEdgeFns[ComposedConstraint.simplify()] =
            EF->composeWith(UserEdgeFn.second);
      }
      return std::make_shared<VarEdgeFunction>(ResultUserEdgeFns);
    }
    llvm::report_fatal_error("found unexpected edge function");
  }

  std::shared_ptr<EdgeFunction<l_t>>
  joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override {
    std::cout << "VarEdgeFunction::joinWith\n";
    if (auto VEF =
            dynamic_cast<VarEdgeFunction<user_l_t> *>(otherFunction.get())) {
      // We need to call user-joinWith for pair-wise equal constraints.
      // Otherwise, we need to add a new entry to the result map.
      //    { <true, a>, <A, b> } x { <true, c>, <!A, d> }
      // leads to:
      //    { <true, c x a>, <A, b>, <!A, d> }
      // initialize with an existing map
      std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>, Z3Less>
          ResultUserEdgeFns = VEF->UserEdgeFns;
      for (auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
        bool FoundConstraint = false;
        for (auto &[InConstraint, InUserEdgeFn] : VEF->UserEdgeFns) {
          std::cout << "z3::eq " << Constraint.to_string() << " <--> "
                    << InConstraint.to_string() << " --> "
                    << z3::eq(Constraint, InConstraint) << '\n';
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
      // unique constraints in VEF->UserEdgeFns are already handled by
      // ResultUserEdgeFns's initialization
      std::cout << "ResultUserEdgeFns.size() --> " << ResultUserEdgeFns.size()
                << '\n';
      return std::make_shared<VarEdgeFunction<user_l_t>>(ResultUserEdgeFns);
    }
    llvm::report_fatal_error("found unexpected edge function");
  }

  bool equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override {
    std::cout << "VarEdgeFunction::equal_to\n";
    // if (auto VEF = dynamic_cast<VarEdgeFunction<user_l_t> *>(other.get())) {
    //   if (UserEdgeFns.size() != VEF->UserEdgeFns.size()) {
    //     return false;
    //   }
    //   for (auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
    //     bool FoundEntry = false;
    //     for (auto &[InConstraint, InUserEdgeFn] : VEF->UserEdgeFns) {
    //       if (z3::eq(Constraint, InConstraint)) {
    //         if (UserEdgeFn->equal_to(InUserEdgeFn)) {
    //           FoundEntry = true;
    //           break;
    //         }
    //       }
    //     }
    //     if (!FoundEntry) {
    //       return false;
    //     }
    //   }
    // }
    // return false;
    return true;
  }
};

} // namespace psr

#endif
