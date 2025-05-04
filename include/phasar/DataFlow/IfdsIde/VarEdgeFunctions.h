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

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/Utils/Logger.h"

#include "llvm/Support/ErrorHandling.h"

#include <map>
#include <memory>
#include <utility>

#include <z3++.h>

namespace psr {

struct Z3Less {
  bool operator()(const z3::expr &Lhs, const z3::expr &Rhs) const {
    return Lhs.id() < Rhs.id();
  }
};

template <typename L> struct VarL : public std::map<z3::expr, L, Z3Less> {
  using std::map<z3::expr, L, Z3Less>::map;
};

template <typename L>
inline bool ContainsZ3Expr(const VarL<L> &M, const z3::expr &E) {
  // TODO: Why cannot we use M.count(E) here?
  bool FoundKey = false;
  for (auto &[Key, Value] : M) {
    if (z3::eq(Key, E)) {
      FoundKey = true;
      break;
    }
  }
  return FoundKey;
}

// template <typename T>
// bool operator==(
//     const std::map<z3::expr, std::shared_ptr<EdgeFunction<T>>, Z3Less> &Lhs,
//     const std::map<z3::expr, std::shared_ptr<EdgeFunction<T>>, Z3Less> &Rhs)
//     {
//   if (Lhs.size() != Rhs.size()) {
//     return false;
//   }
//   for (auto &[LhsConstraint, LhsEF] : Lhs) {
//     bool FoundEntry = false;
//     // TODO: Use Rhs.find(LhsConstraint) ?
//     for (auto &[RhsConstraint, RhsEF] : Rhs) {
//       if (z3::eq(LhsConstraint, RhsConstraint)) {
//         if (&*LhsEF == &*RhsEF || LhsEF->equal_to(RhsEF)) {
//           FoundEntry = true;
//           break;
//         }
//       }
//     }
//     if (!FoundEntry) {
//       return false;
//     }
//   }
//   return true;
// }

template <typename L> class VarEdgeFunction {
public:
  using user_l_t = L;
  using l_t = VarL<L>;

  VarEdgeFunction(EdgeFunction<user_l_t> UserEdgeFn, const z3::expr &Constraint)
      : UserEdgeFns({std::make_pair(Constraint, std::move(UserEdgeFn))}) {

    // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
    //              << "construct VAREdgeFunction with '"
    //              << Constraint.to_string() << "'");
    PHASAR_LOG_LEVEL(DEBUG, "construct VAREdgeFunction: " << this->str());
  }

  VarEdgeFunction(
      const std::map<z3::expr, EdgeFunction<user_l_t>, Z3Less> &UserEdgeFns)
      : UserEdgeFns(UserEdgeFns) {
    PHASAR_LOG_LEVEL(DEBUG,
                     "construct VAREdgeFunction with existing UserEdgeFns");
  }

  VarEdgeFunction(
      std::map<z3::expr, std::shared_ptr<EdgeFunction<user_l_t>>, Z3Less>
          UserEdgeFns)
      : UserEdgeFns(std::move(UserEdgeFns)) {
    PHASAR_LOG_LEVEL(DEBUG,
                     "construct VAREdgeFunction with existing UserEdgeFns");
  }

  l_t computeTarget(ByConstRef<l_t> Source) const {
    PHASAR_LOG_LEVEL(DEBUG, "computeTarget: Source.size(): "
                                << Source.size() << ", UserEdgeFns.size(): "
                                << UserEdgeFns.size());
    auto ResSource = Source;
    for (auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
      PHASAR_LOG_LEVEL(DEBUG, "contains z3 expression '"
                                  << Constraint.to_string() << "' --> "
                                  << ContainsZ3Expr(Source, Constraint));
      if (ContainsZ3Expr(Source, Constraint)) {
        ResSource[Constraint] = UserEdgeFn->computeTarget(Source[Constraint]);
      } else {
        // TODO(sbf): Use bottom() here:
        ResSource[Constraint] = UserEdgeFn->computeTarget(user_l_t{});
      }
    }
    PHASAR_LOG_LEVEL(DEBUG, "ResSource.size(): " << ResSource.size());
    return ResSource;
  }

  static EdgeFunction<l_t> compose(EdgeFunctionRef<VarEdgeFunction> This,
                                   const EdgeFunction<l_t> &secondFunction) {
    PHASAR_LOG_LEVEL(DEBUG, "VarEdgeFunction::composeWith");
    if (auto *VEF =
            dynamic_cast<VarEdgeFunction<user_l_t> *>(secondFunction.get())) {
      PHASAR_LOG_LEVEL(DEBUG, "UserEdgeFns.size(): "
                                  << This->UserEdgeFns.size()
                                  << " --- VEF->UserEdgeFns.size(): "
                                  << VEF->UserEdgeFns.size());
      // We need to compose the constraints as well as the user edge functions.
      // One of the maps will contain one entry only that needs to be composed
      // with the other map (which may contains multiple entries).
      auto &OneEntryMap =
          (VEF->UserEdgeFns.size() == 1) ? VEF->UserEdgeFns : This->UserEdgeFns;
      auto &MulEntryMap =
          (VEF->UserEdgeFns.size() != 1) ? VEF->UserEdgeFns : This->UserEdgeFns;
      PHASAR_LOG_LEVEL(DEBUG,
                       "OneEntryMap.size(): " << OneEntryMap.size()
                                              << " --- MulEntryMap.size(): "
                                              << MulEntryMap.size());
      // access first (and only) element
      auto UserEdgeFn = *OneEntryMap.begin();
      std::map<z3::expr, EdgeFunction<user_l_t>, Z3Less> ResultUserEdgeFns;
      for (auto &[C, EF] : MulEntryMap) {
        // compose constraints and edge functions
        auto ComposedConstraint = C && UserEdgeFn.first;
        ResultUserEdgeFns[ComposedConstraint.simplify()] =
            EF.composeWith(UserEdgeFn.second);
      }
      return VarEdgeFunction(std::move(ResultUserEdgeFns));
    }
    llvm::report_fatal_error("found unexpected edge function");
  }

  static EdgeFunction<l_t> join(EdgeFunctionRef<VarEdgeFunction> This,
                                const EdgeFunction<l_t> otherFunction) {
    PHASAR_LOG_LEVEL(DEBUG, "VarEdgeFunction::joinWith");
    if (auto *VEF =
            dynamic_cast<VarEdgeFunction<user_l_t> *>(otherFunction.get())) {
      // We need to call user-joinWith for pair-wise equal constraints.
      // Otherwise, we need to add a new entry to the result map.
      //    { <true, a>, <A, b> } x { <true, c>, <!A, d> }
      // leads to:
      //    { <true, c x a>, <A, b>, <!A, d> }
      // initialize with an existing map
      std::map<z3::expr, EdgeFunction<user_l_t>, Z3Less> ResultUserEdgeFns =
          VEF->UserEdgeFns;
      for (auto &[Constraint, UserEdgeFn] : This->UserEdgeFns) {
        bool FoundConstraint = false;

        // TODO: Use VEF->UserEdgeFns.find(Constraint) ?
        for (auto &[InConstraint, InUserEdgeFn] : VEF->UserEdgeFns) {
          PHASAR_LOG_LEVEL(DEBUG, "z3::eq "
                                      << Constraint.to_string() << " <--> "
                                      << InConstraint.to_string() << " --> "
                                      << z3::eq(Constraint, InConstraint));
          if (z3::eq(Constraint, InConstraint)) {
            FoundConstraint = true;
            ResultUserEdgeFns[InConstraint] = UserEdgeFn.joinWith(InUserEdgeFn);
          }
        }
        if (!FoundConstraint) {
          ResultUserEdgeFns[Constraint] = UserEdgeFn;
        }
      }
      // unique constraints in VEF->UserEdgeFns are already handled by
      // ResultUserEdgeFns's initialization
      PHASAR_LOG_LEVEL(DEBUG, "ResultUserEdgeFns.size() --> "
                                  << ResultUserEdgeFns.size());
      return VarEdgeFunction<user_l_t>(std::move(ResultUserEdgeFns));
    }
    llvm::report_fatal_error("found unexpected edge function");
  }

  bool operator==(const VarEdgeFunction &other) const {
    PHASAR_LOG_LEVEL(DEBUG, "VarEdgeFunction::equal_to");

    // calling overloaded operator==
    // return UserEdgeFns == other.UserEdgeFns;

    const auto &Lhs = *this;
    const auto &Rhs = other;

    if (Lhs.size() != Rhs.size()) {
      return false;
    }
    for (auto &[LhsConstraint, LhsEF] : Lhs) {
      bool FoundEntry = false;
      // TODO: Use Rhs.find(LhsConstraint) ?
      for (auto &[RhsConstraint, RhsEF] : Rhs) {
        if (z3::eq(LhsConstraint, RhsConstraint)) {
          if (LhsEF == RhsEF) {
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

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const VarEdgeFunction &EF) {
    OS << "(EF: ";
    for (auto &[Constraint, UserEdgeFn] : EF.UserEdgeFns) {
      OS << "<" << Constraint.to_string() << ", " << UserEdgeFn << ">";
    }
    OS << ")";
  }

private:
  std::map<z3::expr, EdgeFunction<user_l_t>, Z3Less> UserEdgeFns;
};

} // namespace psr

#endif
