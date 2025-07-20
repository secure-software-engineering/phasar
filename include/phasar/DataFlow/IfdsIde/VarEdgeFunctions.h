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
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/Twine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TypeName.h"

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

struct Z3Hash {
  size_t operator()(const z3::expr &Expr) const noexcept { return Expr.hash(); }
};

struct Z3Eq {
  size_t operator()(const z3::expr &L, const z3::expr &R) const noexcept {
    return z3::eq(L, R);
  }
};

template <typename L>
struct VarL : public std::unordered_map<z3::expr, L, Z3Hash, Z3Eq> {
  using std::unordered_map<z3::expr, L, Z3Hash, Z3Eq>::unordered_map;
};

template <typename L>
inline bool containsZ3Expr(const VarL<L> &M, const z3::expr &E) {
  // // TODO: Why cannot we use M.count(E) here?
  // bool FoundKey = false;
  // for (auto &[Key, Value] : M) {
  //   if (z3::eq(Key, E)) {
  //     FoundKey = true;
  //     break;
  //   }
  // }
  // return FoundKey;
  return M.count(E);
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

  using map_t =
      std::unordered_map<z3::expr, EdgeFunction<user_l_t>, Z3Hash, Z3Eq>;

  VarEdgeFunction(EdgeFunction<user_l_t> UserEdgeFn, const z3::expr &Constraint)
      : UserEdgeFns({std::make_pair(Constraint, std::move(UserEdgeFn))}) {

    // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
    //              << "construct VAREdgeFunction with '"
    //              << Constraint.to_string() << "'");
    PHASAR_LOG_LEVEL(DEBUG, "construct VAREdgeFunction: " << *this);
  }

  [[nodiscard]] static EdgeFunction<l_t> from(EdgeFunction<user_l_t> UserEdgeFn,
                                              const z3::expr &Constraint) {
    if (Constraint.bool_value() == Z3_lbool::Z3_L_TRUE) {
      if (llvm::isa<EdgeIdentity<user_l_t>>(UserEdgeFn)) {
        return EdgeIdentity<l_t>{};
      }
    }

    return EdgeFunction<l_t>(std::in_place_type<VarEdgeFunction>,
                             std::move(UserEdgeFn), Constraint);
  }

  VarEdgeFunction(map_t UserEdgeFns) : UserEdgeFns(std::move(UserEdgeFns)) {
    PHASAR_LOG_LEVEL(DEBUG,
                     "construct VAREdgeFunction with existing UserEdgeFns");
  }

  // TODO(sbf): isConstant()

  l_t computeTarget(const l_t &Source) const {
    PHASAR_LOG_LEVEL(DEBUG, "computeTarget: Source.size(): "
                                << Source.size() << ", UserEdgeFns.size(): "
                                << UserEdgeFns.size());
    auto ResSource = Source;
    for (auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
      PHASAR_LOG_LEVEL(DEBUG, "contains z3 expression '"
                                  << Constraint.to_string() << "' --> "
                                  << containsZ3Expr(Source, Constraint));
      if (auto It = Source.find(Constraint); It != Source.end()) {
        ResSource[Constraint] = UserEdgeFn.computeTarget(It->second);
      } else {
        if constexpr (HasJoinLatticeTraits<user_l_t>) {
          ResSource[Constraint] =
              UserEdgeFn.computeTarget(JoinLatticeTraits<user_l_t>::bottom());
        } else {
          // Use the default-constructed value as fallback; we probably may want
          // to use a different fallback value here eventually...
          ResSource[Constraint] = UserEdgeFn.computeTarget(user_l_t{});
        }
      }
    }
    PHASAR_LOG_LEVEL(DEBUG, "ResSource.size(): " << ResSource.size());
    return ResSource;
  }

  static EdgeFunction<l_t> compose(EdgeFunctionRef<VarEdgeFunction> This,
                                   const EdgeFunction<l_t> &SecondFunction) {
    if (llvm::isa<EdgeIdentity<l_t>>(SecondFunction)) {
      return This;
    }

    if (SecondFunction.isConstant()) {
      return SecondFunction;
    }

    PHASAR_LOG_LEVEL(DEBUG, "VarEdgeFunction::composeWith");
    const VarEdgeFunction *VEF =
        llvm::dyn_cast<VarEdgeFunction>(SecondFunction);
    if (!VEF) {
      llvm::report_fatal_error("found unexpected second edge function: " +
                               llvm::Twine(to_string(SecondFunction)));
    }

    // TODO(sbf): May want to have a specialization for
    // SecondFunction.isConstant(), once we have that information

    map_t ResultUserEdgeFns;
    ResultUserEdgeFns.reserve(This->UserEdgeFns.size());
    for (const auto &[OtherConstraint, OtherEF] : VEF->UserEdgeFns) {
      for (const auto &[ThisConstraint, ThisEF] : This->UserEdgeFns) {
        auto ComposedConstraint =
            (ThisConstraint && OtherConstraint).simplify();
        auto ComposedEF = ThisEF.composeWith(OtherEF);
        auto [It, Inserted] = ResultUserEdgeFns.try_emplace(
            ComposedConstraint, std::move(ComposedEF));
        if (!Inserted) {
          It->second = It->second.joinWith(std::move(ComposedEF));
        }
      }
    }

    // PHASAR_LOG_LEVEL(DEBUG,
    //                  "UserEdgeFns.size(): " << This->UserEdgeFns.size()
    //                                         << " --- VEF->UserEdgeFns.size():
    //                                         "
    //                                         << VEF->UserEdgeFns.size());
    // // We need to compose the constraints as well as the user edge functions.
    // // One of the maps will contain one entry only that needs to be composed
    // // with the other map (which may contain multiple entries).
    // auto &OneEntryMap =
    //     (VEF->UserEdgeFns.size() == 1) ? VEF->UserEdgeFns :
    //     This->UserEdgeFns;
    // auto &MulEntryMap =
    //     (VEF->UserEdgeFns.size() != 1) ? VEF->UserEdgeFns :
    //     This->UserEdgeFns;
    // PHASAR_LOG_LEVEL(DEBUG,
    //                  "OneEntryMap.size(): " << OneEntryMap.size()
    //                                         << " --- MulEntryMap.size(): "
    //                                         << MulEntryMap.size());
    // // access first (and only) element
    // auto UserEdgeFn = *OneEntryMap.begin();

    // for (auto &[C, EF] : MulEntryMap) {
    //   // compose constraints and edge functions
    //   auto ComposedConstraint = C && UserEdgeFn.first;

    //   ResultUserEdgeFns[ComposedConstraint.simplify()] =
    //       EF.composeWith(UserEdgeFn.second);
    // }
    return EdgeFunction<l_t>(std::in_place_type<VarEdgeFunction>,
                             std::move(ResultUserEdgeFns));
  }

  static EdgeFunction<l_t> join(EdgeFunctionRef<VarEdgeFunction> This,
                                const EdgeFunction<l_t> OtherFunction) {
    PHASAR_LOG_LEVEL(DEBUG, "VarEdgeFunction::joinWith");

    if (auto Dflt = psr::defaultJoinOrNull<l_t>(This, OtherFunction)) {
      return Dflt;
    }
    if (llvm::isa<EdgeIdentity<l_t>>(OtherFunction)) {
      // TODO: Shouldn't the BottomValue rather be a VarL containing <true,
      // Bottom>, as defined in the IDEVarTabulationProblem?
      return AllBottom<l_t>{VarL<user_l_t>{}};
    }

    const auto *VEF =
        OtherFunction.template dyn_cast<VarEdgeFunction<user_l_t>>();
    if (!VEF) {
      llvm::report_fatal_error("found unexpected other edge function: " +
                               llvm::Twine(to_string(OtherFunction)));
    }

    // We need to call user-joinWith for pair-wise equal constraints.
    // Otherwise, we need to add a new entry to the result map.
    //    { <true, a>, <A, b> } x { <true, c>, <!A, d> }
    // leads to:
    //    { <true, c x a>, <A, b>, <!A, d> }
    // initialize with an existing map
    map_t ResultUserEdgeFns = This->UserEdgeFns;
    bool Changed = false;
    for (auto &[Constraint, UserEdgeFn] : VEF->UserEdgeFns) {
      auto [It, Inserted] =
          ResultUserEdgeFns.try_emplace(Constraint, UserEdgeFn);

      if (Inserted) {
        Changed = true;
      } else {
        auto NewEF = It->second.joinWith(UserEdgeFn);
        if (NewEF != It->second) {
          It->second = std::move(NewEF);
          Changed = true; // Modified entry
        }
      }

      // bool FoundConstraint = false;
      // for (auto &[InConstraint, InUserEdgeFn] : VEF->UserEdgeFns) {
      //   PHASAR_LOG_LEVEL(DEBUG, "z3::eq "
      //                               << Constraint.to_string() << " <--> "
      //                               << InConstraint.to_string() << " --> "
      //                               << z3::eq(Constraint, InConstraint));
      //   if (z3::eq(Constraint, InConstraint)) {
      //     FoundConstraint = true;
      //     ResultUserEdgeFns[InConstraint] =
      //     UserEdgeFn.joinWith(InUserEdgeFn);
      //   }
      // }
      // if (!FoundConstraint) {
      //   ResultUserEdgeFns[Constraint] = UserEdgeFn;
      // }
    }

    if (!Changed) {
      return This;
    }

    // unique constraints in VEF->UserEdgeFns are already handled by
    // ResultUserEdgeFns's initialization
    PHASAR_LOG_LEVEL(DEBUG, "ResultUserEdgeFns.size() --> "
                                << ResultUserEdgeFns.size());
    return EdgeFunction<l_t>(std::in_place_type<VarEdgeFunction>,
                             std::move(ResultUserEdgeFns));
  }

  bool operator==(const VarEdgeFunction &Other) const {
    PHASAR_LOG_LEVEL(DEBUG, "VarEdgeFunction::equal_to");

    // calling overloaded operator==
    // return UserEdgeFns == other.UserEdgeFns;

    // const auto &Lhs = *this;
    // const auto &Rhs = other;

    // if (Lhs.size() != Rhs.size()) {
    //   return false;
    // }
    // for (auto &[LhsConstraint, LhsEF] : Lhs) {
    //   bool FoundEntry = false;
    //   // TODO: Use Rhs.find(LhsConstraint) ?
    //   for (auto &[RhsConstraint, RhsEF] : Rhs) {
    //     if (z3::eq(LhsConstraint, RhsConstraint)) {
    //       if (LhsEF == RhsEF) {
    //         FoundEntry = true;
    //         break;
    //       }
    //     }
    //   }
    //   if (!FoundEntry) {
    //     return false;
    //   }
    // }
    // return true;
    return UserEdgeFns == Other.UserEdgeFns;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const VarEdgeFunction &EF) {
    OS << "(EF: ";
    for (auto &[Constraint, UserEdgeFn] : EF.UserEdgeFns) {
      OS << "<" << Constraint.to_string() << ", " << UserEdgeFn << ">";
    }
    return OS << ")";
  }

private:
  map_t UserEdgeFns;
};

} // namespace psr

#endif
