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

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/Utils/Logger.h"

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

template <typename T>
bool operator==(
    const std::map<z3::expr, std::shared_ptr<EdgeFunction<T>>, Z3Less> &Lhs,
    const std::map<z3::expr, std::shared_ptr<EdgeFunction<T>>, Z3Less> &Rhs) {
  if (Lhs.size() != Rhs.size()) {
    return false;
  }
  for (auto &[LhsConstraint, LhsEF] : Lhs) {
    bool FoundEntry = false;
    for (auto &[RhsConstraint, RhsEF] : Rhs) {
      if (z3::eq(LhsConstraint, RhsConstraint)) {
        if (&*LhsEF == &*RhsEF || LhsEF->equal_to(RhsEF)) {
          FoundEntry = true;
          break;
        }
      }
    }
    if (!FoundEntry) {
      return false;
    }
  }
  return true;
}

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
  VarEdgeFunction(const std::shared_ptr<EdgeFunction<user_l_t>> &UserEdgeFn,
                  const z3::expr &Constraint)
      : UserEdgeFns({std::make_pair(Constraint, UserEdgeFn)}) {

    // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
    //              << "construct VAREdgeFunction with '"
    //              << Constraint.to_string() << "'");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "construct VAREdgeFunction: " << this->str());
  }

  VarEdgeFunction(const std::shared_ptr<EdgeFunction<user_l_t>> &UserEdgeFn,
                  z3::expr &&Constraint)
      : UserEdgeFns({std::make_pair(std::move(Constraint), UserEdgeFn)}) {

    // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
    //              << "construct VAREdgeFunction with '"
    //              << Constraint.to_string() << "'");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "construct VAREdgeFunction: " << this->str());
  }

  VarEdgeFunction(
      const std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>, Z3Less>
          &UserEdgeFns)
      : UserEdgeFns(UserEdgeFns) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "construct VAREdgeFunction with existing UserEdgeFns");
  }

  VarEdgeFunction(std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>,
                           Z3Less> &&UserEdgeFns)
      : UserEdgeFns(std::move(UserEdgeFns)) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "construct VAREdgeFunction with existing UserEdgeFns");
  }

  ~VarEdgeFunction() override = default;

  l_t computeTarget(l_t Source) override {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "computeTarget: Source.size(): " << Source.size()
                  << ", UserEdgeFns.size(): " << UserEdgeFns.size());
    // TODO: Do we really need to copy Source here? It _is_ already a copy and
    // each Constraint occurs only once in UserEdgeFns...
    auto ResSource = Source;
    for (auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "contains z3 expression '" << Constraint.to_string()
                    << "' --> " << ContainsZ3Expr(Source, Constraint));
      if (ContainsZ3Expr(Source, Constraint)) {
        ResSource[Constraint] = UserEdgeFn->computeTarget(Source[Constraint]);
      } else {
        ResSource[Constraint] = UserEdgeFn->computeTarget(user_l_t{});
      }
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "ResSource.size(): " << ResSource.size());
    return ResSource;
  }

  std::shared_ptr<EdgeFunction<l_t>>
  composeWith(std::shared_ptr<EdgeFunction<l_t>> secondFunction) override {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "VarEdgeFunction::composeWith");
    if (auto *VEF =
            dynamic_cast<VarEdgeFunction<user_l_t> *>(secondFunction.get())) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "UserEdgeFns.size(): " << UserEdgeFns.size()
                    << " --- VEF->UserEdgeFns.size(): "
                    << VEF->UserEdgeFns.size());
      // We need to compose the constraints as well as the user edge functions.
      // One of the maps will contain one entry only that needs to be composed
      // with the other map (which may contains multiple entries).
      auto &OneEntryMap =
          (VEF->UserEdgeFns.size() == 1) ? VEF->UserEdgeFns : UserEdgeFns;
      auto &MulEntryMap =
          (VEF->UserEdgeFns.size() != 1) ? VEF->UserEdgeFns : UserEdgeFns;
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "OneEntryMap.size(): " << OneEntryMap.size()
                    << " --- MulEntryMap.size(): " << MulEntryMap.size());
      // access first (and only) element
      auto UserEdgeFn = *OneEntryMap.begin();
      std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>, Z3Less>
          ResultUserEdgeFns;
      for (auto &[C, EF] : MulEntryMap) {
        // compose constraints and edge functions
        auto ComposedConstraint = C && UserEdgeFn.first;
        ResultUserEdgeFns[ComposedConstraint.simplify()] =
            EF->composeWith(UserEdgeFn.second);
      }
      return std::make_shared<VarEdgeFunction>(std::move(ResultUserEdgeFns));
    }
    llvm::report_fatal_error("found unexpected edge function");
  }

  std::shared_ptr<EdgeFunction<l_t>>
  joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "VarEdgeFunction::joinWith");
    if (auto *VEF =
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
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "z3::eq " << Constraint.to_string() << " <--> "
                        << InConstraint.to_string() << " --> "
                        << z3::eq(Constraint, InConstraint));
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
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "ResultUserEdgeFns.size() --> "
                    << ResultUserEdgeFns.size());
      return std::make_shared<VarEdgeFunction<user_l_t>>(
          std::move(ResultUserEdgeFns));
    }
    llvm::report_fatal_error("found unexpected edge function");
  }

  bool equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "VarEdgeFunction::equal_to");
    if (auto *VEF = dynamic_cast<VarEdgeFunction<user_l_t> *>(other.get())) {
      // calling overloaded operator==
      return UserEdgeFns == VEF->UserEdgeFns;
    }
    return false;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "(EF: ";
    for (auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
      OS << "<" << Constraint.to_string() << ", ";
      UserEdgeFn->print(OS);
      OS << ">";
    }
    OS << ")";
  }
};

} // namespace psr

#endif
