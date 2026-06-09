#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/DefaultValue.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/NonNullPtr.h"

#include "llvm/ADT/ArrayRef.h"

namespace psr {

/// \brief A simple wrapper over a pointer to an IFDS/IDE problem.
///
/// Useful for adding intermediate layers, e.g., IFDS->IDE translation, caching,
/// etc.
template <typename ProblemTy> class IDEProblemWrapper {
public:
  using ProblemAnalysisDomain = typename ProblemTy::ProblemAnalysisDomain;
  using d_t = typename ProblemAnalysisDomain::d_t;
  using n_t = typename ProblemAnalysisDomain::n_t;
  using f_t = typename ProblemAnalysisDomain::f_t;
  using t_t = typename ProblemAnalysisDomain::t_t;
  using v_t = typename ProblemAnalysisDomain::v_t;
  using l_t = typename ProblemAnalysisDomain::l_t;
  using i_t = typename ProblemAnalysisDomain::i_t;
  using db_t = typename ProblemAnalysisDomain::db_t;

  constexpr IDEProblemWrapper(NonNullPtr<ProblemTy> Problem) noexcept
      : Problem(Problem) {}
  constexpr IDEProblemWrapper(ProblemTy *Problem) noexcept : Problem(Problem) {}

  // --- IFDSProblem:

  [[nodiscard]] constexpr bool isZeroValue(ByConstRef<d_t> Fact) const {
    if constexpr (requires { Problem->isZeroValue(Fact); }) {
      return Problem->isZeroValue(Fact);
    } else {
      return Fact == getZeroValue();
    }
  }

  [[nodiscard]] constexpr decltype(auto) getZeroValue() const {
    return Problem->getZeroValue();
  }

  [[nodiscard]] constexpr decltype(auto) initialSeeds() {
    return Problem->initialSeeds();
  }

  [[nodiscard]] constexpr ProjectIRDBConstPtr auto getProjectIRDB() const {
    return Problem->getProjectIRDB();
  }

  [[nodiscard]] constexpr decltype(auto) getEntryPoints() const {
    return Problem->getEntryPoints();
  }

  // --- FlowFunctionFactory:

  using container_type = typename ProblemTy::container_type;
  using FlowFunctionPtrType = typename ProblemTy::FlowFunctionPtrType;

  [[nodiscard]] constexpr decltype(auto)
  getNormalFlowFunction(ByConstRef<n_t> Curr, ByConstRef<n_t> Succ) {
    return Problem->getNormalFlowFunction(Curr, Succ);
  }

  [[nodiscard]] constexpr decltype(auto)
  getCallFlowFunction(ByConstRef<n_t> Curr, ByConstRef<f_t> CalleeFun) {
    return Problem->getCallFlowFunction(Curr, CalleeFun);
  }

  [[nodiscard]] constexpr decltype(auto)
  getRetFlowFunction(ByConstRef<n_t> CallSite, ByConstRef<f_t> CalleeFun,
                     ByConstRef<n_t> ExitInst, ByConstRef<n_t> RetSite) {
    return Problem->getRetFlowFunction(CallSite, CalleeFun, ExitInst, RetSite);
  }

  [[nodiscard]] constexpr decltype(auto)
  getCallToRetFlowFunction(ByConstRef<n_t> CallSite, ByConstRef<n_t> RetSite,
                           llvm::ArrayRef<f_t> Callees) {
    return Problem->getCallToRetFlowFunction(CallSite, RetSite, Callees);
  }

  [[nodiscard]] constexpr decltype(auto)
  getSummaryFlowFunction(ByConstRef<n_t> Curr, ByConstRef<f_t> CalleeFun) {
    if constexpr (requires {
                    Problem->getSummaryFlowFunction(Curr, CalleeFun);
                  }) {
      return Problem->getSummaryFlowFunction(Curr, CalleeFun);
    } else {
      return nullptr;
    }
  }

  // --- EdgeFunctionFactory:

  using EdgeFunctionType = typename ProblemTy::EdgeFunctionType;

  [[nodiscard]] constexpr decltype(auto)
  getNormalEdgeFunction(ByConstRef<n_t> Curr, ByConstRef<d_t> CurrNode,
                        ByConstRef<n_t> Succ, ByConstRef<d_t> SuccNode) {
    return Problem->getNormalEdgeFunction(Curr, CurrNode, Succ, SuccNode);
  }

  [[nodiscard]] constexpr decltype(auto)
  getCallEdgeFunction(ByConstRef<n_t> CallSite, ByConstRef<d_t> CSNode,
                      ByConstRef<f_t> CalleeFun, ByConstRef<d_t> CalleeNode) {
    return Problem->getCallEdgeFunction(CallSite, CSNode, CalleeFun,
                                        CalleeNode);
  }

  [[nodiscard]] constexpr decltype(auto)
  getReturnEdgeFunction(ByConstRef<n_t> CallSite, ByConstRef<f_t> CalleeFun,
                        ByConstRef<n_t> ExitInst, ByConstRef<d_t> ExitNode,
                        ByConstRef<n_t> RetSite, ByConstRef<d_t> RSNode) {
    return Problem->getReturnEdgeFunction(CallSite, CalleeFun, ExitInst,
                                          ExitNode, RetSite, RSNode);
  }

  [[nodiscard]] constexpr decltype(auto)
  getCallToRetEdgeFunction(ByConstRef<n_t> CallSite, ByConstRef<d_t> CSNode,
                           ByConstRef<n_t> RetSite, ByConstRef<d_t> RSNode,
                           llvm::ArrayRef<f_t> Callees) {
    return Problem->getCallToRetEdgeFunction(CallSite, CSNode, RetSite, RSNode,
                                             Callees);
  }

  [[nodiscard]] constexpr decltype(auto)
  getSummaryEdgeFunction(ByConstRef<n_t> Curr, ByConstRef<d_t> CurrNode,
                         ByConstRef<n_t> Succ, ByConstRef<d_t> SuccNode) {
    if constexpr (requires {
                    Problem->getSummaryEdgeFunction(Curr, CurrNode, Succ,
                                                    SuccNode);
                  }) {

      return Problem->getSummaryEdgeFunction(Curr, CurrNode, Succ, SuccNode);
    } else {
      return getDefaultValue<EdgeFunctionType>();
    }
  }

  // --- IsJoinLattice:

  [[nodiscard]] constexpr decltype(auto) topElement() {
    if constexpr (requires { Problem->topElement(); }) {
      return Problem->topElement();
    } else {
      return JoinLatticeTraits<l_t>::top();
    }
  }

  [[nodiscard]] constexpr decltype(auto) bottomElement() {
    if constexpr (requires { Problem->bottomElement(); }) {
      return Problem->bottomElement();
    } else {
      return JoinLatticeTraits<l_t>::bottom();
    }
  }

  [[nodiscard]] constexpr decltype(auto) join(auto &&L, auto &&R) {
    if constexpr (requires(l_t Val) { Problem->join(Val, Val); }) {
      return Problem->join(PSR_FWD(L), PSR_FWD(R));
    } else {
      return JoinLatticeTraits<l_t>::join(PSR_FWD(L), PSR_FWD(R));
    }
  }

  // --- IsSemiRing:

  [[nodiscard]] constexpr decltype(auto)
  extend(IsEdgeFunctionFor<l_t> auto &&First,
         IsEdgeFunctionFor<l_t> auto &&Second) {
    return Problem->extend(PSR_FWD(First), PSR_FWD(Second));
  }

  [[nodiscard]] constexpr decltype(auto)
  combine(IsEdgeFunctionFor<l_t> auto &&First,
          IsEdgeFunctionFor<l_t> auto &&Second) {
    return Problem->combine(PSR_FWD(First), PSR_FWD(Second));
  }

  [[nodiscard]] constexpr decltype(auto) identity() {
    if constexpr (requires { Problem->identity(); }) {
      return Problem->identity();
    } else {
      return EdgeIdentity<l_t>{};
    }
  }

  [[nodiscard]] constexpr decltype(auto) allTopFunction() {
    if constexpr (requires { Problem->allTopFunction(); }) {
      return Problem->allTopFunction();
    } else if constexpr (HasJoinLatticeTraits<l_t>) {
      return AllTop<l_t>{};
    } else {
      return AllTop<l_t>{topElement()};
    }
  }

protected:
  NonNullPtr<ProblemTy> Problem;
};

} // namespace psr
