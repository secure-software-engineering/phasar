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
#include "phasar/DataFlow/IfdsIde/IDEProblem.h"
#include "phasar/DataFlow/IfdsIde/IFDSProblemWrapper.h"
#include "phasar/DataFlow/IfdsIde/IfdsToIdeProblemAdapter.h"
#include "phasar/Utils/ByRef.h"
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
    return this->Problem->getSummaryEdgeFunction(Curr, CurrNode, Succ,
                                                 SuccNode);
  }

  // --- IsJoinLattice:

  [[nodiscard]] constexpr decltype(auto) topElement() {
    return this->Problem->topElement();
  }

  [[nodiscard]] constexpr decltype(auto) bottomElement() {
    return this->Problem->bottomElement();
  }

  [[nodiscard]] constexpr decltype(auto) join(auto &&L, auto &&R) {
    return this->Problem->join(PSR_FWD(L), PSR_FWD(R));
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
    return this->Problem->identity();
  }

  [[nodiscard]] constexpr decltype(auto) allTopFunction()
    requires HasAllTopFunction<ProblemTy>
  {
    return this->Problem->allTopFunction();
  }
};

template <IFDSProblem ProblemTy>
class IfdsIdeProblemWrapper
    : public IfdsToIdeProblemAdapter<IFDSProblemWrapper<ProblemTy>> {
public:
  using IfdsToIdeProblemAdapter<
      IFDSProblemWrapper<ProblemTy>>::IfdsToIdeProblemAdapter;

  using IDEProblemTy = IfdsIdeProblemWrapper;

  [[nodiscard]] constexpr IDEProblem auto &ideProblem() noexcept {
    return *this;
  }
};

template <IDEProblem ProblemTy>
class IfdsIdeProblemWrapper<ProblemTy> : public IDEProblemWrapper<ProblemTy> {
public:
  using IDEProblemWrapper<ProblemTy>::IDEProblemWrapper;

  using IDEProblemTy = ProblemTy;

  [[nodiscard]] constexpr IDEProblem auto &ideProblem() noexcept {
    return this->base();
  }
};

} // namespace psr
