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

#include "z3++.h"

#include <map>
#include <memory>
#include <utility>

namespace psr {

struct Z3Key {
  z3::expr Expr;

  Z3Key(const z3::expr &Expr) : Expr(Expr) {}

  bool operator==(const Z3Key &Other) const noexcept {
    return z3::eq(Expr, Other.Expr);
  }

  friend size_t hash_value(const Z3Key &Ky) noexcept { return Ky.Expr.hash(); }
};

[[nodiscard]] inline std::string to_string(const Z3Key &Ky) {
  return Ky.Expr.to_string();
}
inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const Z3Key &Ky) {
  return OS << to_string(Ky);
}

// struct Z3Less {
//   bool operator()(const z3::expr &Lhs, const z3::expr &Rhs) const {
//     return Lhs.id() < Rhs.id();
//   }
// };

// struct Z3Hash {
//   size_t operator()(const z3::expr &Expr) const noexcept {
//     llvm::errs() << "Z3Hash: " << Expr.to_string() << ": " << Expr.hash()
//                  << '\n';
//     return Expr.hash();
//   }
// };

// struct Z3Eq {
//   size_t operator()(const z3::expr &L, const z3::expr &R) const noexcept {
//     auto Ret = z3::eq(L, R);
//     if (!L.is_true() && !R.is_true()) {
//       llvm::errs() << "Z3Eq: " << L.to_string() << " == " << R.to_string()
//                    << " ==> " << Ret << '\n';
//     }
//     return Ret;
//   }
// };

} // namespace psr

namespace std {
template <> struct hash<psr::Z3Key> {
  size_t operator()(const psr::Z3Key &Ky) const noexcept {
    return hash_value(Ky);
  }
};
} // namespace std

namespace psr {

template <typename L> struct VarL : public std::unordered_map<Z3Key, L> {
  using std::unordered_map<Z3Key, L>::unordered_map;
};

template <typename L> class VarEdgeFunction {
public:
  using user_l_t = L;
  using l_t = VarL<L>;

  using map_t = std::unordered_map<Z3Key, EdgeFunction<user_l_t>>;

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
    // auto ResSource = Source;
    l_t ResSource{};
    for (const auto &[Constraint, UserEdgeFn] : UserEdgeFns) {
      PHASAR_LOG_LEVEL(DEBUG, "contains z3 expression '"
                                  << Constraint << "' --> "
                                  << Source.count(Constraint));
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

    if (ResSource.empty()) {
      ResSource = Source;
    } else {
      // Add all missing facts
      for (const auto &[Constraint, IncomingEF] : Source) {
        if (Constraint.Expr.is_true() || Constraint.Expr.is_false()) {
          continue;
        }
        ResSource.try_emplace(Constraint, IncomingEF);
      }
    }

    PHASAR_LOG_LEVEL(DEBUG, "ResSource.size(): " << ResSource.size());
    // llvm::errs() << "[computeTarget]: " << *this << '(' << LToString(Source)
    //              << ") = " << LToString(ResSource) << '\n';
    return ResSource;
  }

  static EdgeFunction<l_t> compose(EdgeFunctionRef<VarEdgeFunction> This,
                                   const EdgeFunction<l_t> &SecondFunction) {
    auto Ret = [&]() -> EdgeFunction<l_t> {
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
          z3::expr ComposedConstraint =
              (ThisConstraint.Expr && OtherConstraint.Expr).simplify();
          if (ComposedConstraint.is_false()) {
            continue;
          }
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
      //                                         << " ---
      //                                         VEF->UserEdgeFns.size():
      //                                         "
      //                                         << VEF->UserEdgeFns.size());
      // // We need to compose the constraints as well as the user edge
      // functions.
      // // One of the maps will contain one entry only that needs to be
      // composed
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
    }();
    // if (!SecondFunction.template isa<EdgeIdentity<l_t>>()) {
    //   llvm::errs() << "COMPOSE:\n  This:   " << *This
    //                << "\n  Second: " << SecondFunction << "\n  ==> " << Ret
    //                << '\n';
    // }
    return Ret;
  }

  static EdgeFunction<l_t> join(EdgeFunctionRef<VarEdgeFunction> This,
                                const EdgeFunction<l_t> OtherFunction) {
    PHASAR_LOG_LEVEL(DEBUG, "VarEdgeFunction::joinWith");

    // llvm::errs() << "JOIN:\n  This:  " << *This
    //              << "\n  Other: " << OtherFunction << '\n';

    if (auto Dflt = psr::defaultJoinOrNull<l_t>(This, OtherFunction)) {
      // llvm::errs() << "  ==> (default): " << Dflt << '\n';
      return Dflt;
    }
    if (llvm::isa<EdgeIdentity<l_t>>(OtherFunction)) {
      // llvm::errs() << "  ==> AllBottom\n";
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
      // llvm::errs() << "  ==> (unchanged): " << *This << '\n';
      return This;
    }

    // unique constraints in VEF->UserEdgeFns are already handled by
    // ResultUserEdgeFns's initialization
    PHASAR_LOG_LEVEL(DEBUG, "ResultUserEdgeFns.size() --> "
                                << ResultUserEdgeFns.size());
    auto Ret = EdgeFunction<l_t>(std::in_place_type<VarEdgeFunction>,
                                 std::move(ResultUserEdgeFns));

    // llvm::errs() << "  ==> " << Ret << '\n';
    return Ret;
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
      OS << "<" << Constraint << ", " << UserEdgeFn << ">";
    }
    return OS << ")";
  }

private:
  map_t UserEdgeFns;
};

} // namespace psr

#endif
