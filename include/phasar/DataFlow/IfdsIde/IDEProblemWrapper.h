#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/IDEProblem.h"
#include "phasar/DataFlow/IfdsIde/IFDSProblemWrapper.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/DefaultValue.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/SemiRing.h"

#include "llvm/ADT/ArrayRef.h"

namespace psr {

/// \brief A simple wrapper over a pointer to an IFDS/IDE problem.
///
/// Useful for adding intermediate layers, e.g., IFDS->IDE translation, caching,
/// etc.
template <IDEProblem ProblemTy>
class IDEProblemWrapper : public IFDSProblemWrapper<ProblemTy> {
  using base_t = IFDSProblemWrapper<ProblemTy>;

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

  using base_t::base_t;

  // --- EdgeFunctionFactory:

  using EdgeFunctionType = typename ProblemTy::EdgeFunctionType;

  [[nodiscard]] constexpr decltype(auto)
  getNormalEdgeFunction(ByConstRef<n_t> Curr, ByConstRef<d_t> CurrNode,
                        ByConstRef<n_t> Succ, ByConstRef<d_t> SuccNode) {
    return this->Problem->getNormalEdgeFunction(Curr, CurrNode, Succ, SuccNode);
  }

  [[nodiscard]] constexpr decltype(auto)
  getCallEdgeFunction(ByConstRef<n_t> CallSite, ByConstRef<d_t> CSNode,
                      ByConstRef<f_t> CalleeFun, ByConstRef<d_t> CalleeNode) {
    return this->Problem->getCallEdgeFunction(CallSite, CSNode, CalleeFun,
                                              CalleeNode);
  }

  [[nodiscard]] constexpr decltype(auto)
  getReturnEdgeFunction(ByConstRef<n_t> CallSite, ByConstRef<f_t> CalleeFun,
                        ByConstRef<n_t> ExitInst, ByConstRef<d_t> ExitNode,
                        ByConstRef<n_t> RetSite, ByConstRef<d_t> RSNode) {
    return this->Problem->getReturnEdgeFunction(CallSite, CalleeFun, ExitInst,
                                                ExitNode, RetSite, RSNode);
  }

  [[nodiscard]] constexpr decltype(auto)
  getCallToRetEdgeFunction(ByConstRef<n_t> CallSite, ByConstRef<d_t> CSNode,
                           ByConstRef<n_t> RetSite, ByConstRef<d_t> RSNode,
                           llvm::ArrayRef<f_t> Callees) {
    return this->Problem->getCallToRetEdgeFunction(CallSite, CSNode, RetSite,
                                                   RSNode, Callees);
  }

  [[nodiscard]] constexpr decltype(auto)
  getSummaryEdgeFunction(ByConstRef<n_t> Curr, ByConstRef<d_t> CurrNode,
                         ByConstRef<n_t> Succ, ByConstRef<d_t> SuccNode) {
    if constexpr (requires {
                    this->Problem->getSummaryEdgeFunction(Curr, CurrNode, Succ,
                                                          SuccNode);
                  }) {

      return this->Problem->getSummaryEdgeFunction(Curr, CurrNode, Succ,
                                                   SuccNode);
    } else {
      return getDefaultValue<EdgeFunctionType>();
    }
  }

  // --- IsJoinLattice:

  [[nodiscard]] constexpr decltype(auto) topElement() {
    if constexpr (requires { this->Problem->topElement(); }) {
      return this->Problem->topElement();
    } else {
      return JoinLatticeTraits<l_t>::top();
    }
  }

  [[nodiscard]] constexpr decltype(auto) bottomElement() {
    if constexpr (requires { this->Problem->bottomElement(); }) {
      return this->Problem->bottomElement();
    } else {
      return JoinLatticeTraits<l_t>::bottom();
    }
  }

  [[nodiscard]] constexpr decltype(auto) join(auto &&L, auto &&R) {
    if constexpr (requires(l_t Val) { this->Problem->join(Val, Val); }) {
      return this->Problem->join(PSR_FWD(L), PSR_FWD(R));
    } else {
      return JoinLatticeTraits<l_t>::join(PSR_FWD(L), PSR_FWD(R));
    }
  }

  // --- IsSemiRing:

  [[nodiscard]] constexpr decltype(auto)
  extend(IsEdgeFunctionFor<l_t> auto &&First,
         IsEdgeFunctionFor<l_t> auto &&Second) {
    return this->Problem->extend(PSR_FWD(First), PSR_FWD(Second));
  }

  [[nodiscard]] constexpr decltype(auto)
  combine(IsEdgeFunctionFor<l_t> auto &&First,
          IsEdgeFunctionFor<l_t> auto &&Second) {
    return this->Problem->combine(PSR_FWD(First), PSR_FWD(Second));
  }

  [[nodiscard]] constexpr decltype(auto) identity() {
    if constexpr (requires { this->Problem->identity(); }) {
      return this->Problem->identity();
    } else {
      return EdgeIdentity<l_t>{};
    }
  }

  [[nodiscard]] constexpr decltype(auto) allTopFunction() {
    if constexpr (HasAllTopFunction<ProblemTy>) {
      return this->Problem->allTopFunction();
    } else if constexpr (HasJoinLatticeTraits<l_t>) {
      return AllTop<l_t>{};
    } else {
      return AllTop<l_t>{topElement()};
    }
  }
};

} // namespace psr
