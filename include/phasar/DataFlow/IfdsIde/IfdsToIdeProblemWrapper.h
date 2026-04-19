#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/IDEProblemWrapper.h"
#include "phasar/DataFlow/IfdsIde/IFDSProblem.h"
#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/NonNullPtr.h"

#include <type_traits>

namespace psr {
template <typename Base> class IfdsToIdeProblemWrapper : public Base {
public:
  using Base::Base;

  using typename Base::d_t;
  using typename Base::f_t;
  using typename Base::n_t;

  using l_t = BinaryDomain;
  using EdgeFunctionType = EdgeIdentity<l_t>;

  [[nodiscard]] constexpr decltype(auto)
  getNormalEdgeFunction(ByConstRef<n_t> /*Curr*/, ByConstRef<d_t> /*CurrNode*/,
                        ByConstRef<n_t> /*Succ*/,
                        ByConstRef<d_t> /*SuccNode*/) {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr decltype(auto)
  getCallEdgeFunction(ByConstRef<n_t> /*CallSite*/, ByConstRef<d_t> /*CSNode*/,
                      ByConstRef<f_t> /*CalleeFun*/,
                      ByConstRef<d_t> /*CalleeNode*/) {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr decltype(auto) getReturnEdgeFunction(
      ByConstRef<n_t> /*CallSite*/, ByConstRef<f_t> /*CalleeFun*/,
      ByConstRef<n_t> /*ExitInst*/, ByConstRef<d_t> /*ExitNode*/,
      ByConstRef<n_t> /*RetSite*/, ByConstRef<d_t> /*RSNode*/) {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr decltype(auto) getCallToRetEdgeFunction(
      ByConstRef<n_t> /*CallSite*/, ByConstRef<d_t> /*CSNode*/,
      ByConstRef<n_t> /*RetSite*/, ByConstRef<d_t> /*RSNode*/,
      llvm::ArrayRef<f_t> /*Callees*/) {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr decltype(auto)
  getSummaryEdgeFunction(ByConstRef<n_t> /*Curr*/, ByConstRef<d_t> /*CurrNode*/,
                         ByConstRef<n_t> /*Succ*/,
                         ByConstRef<d_t> /*SuccNode*/) {
    return EdgeFunctionType{};
  }

  // --- IsJoinLattice:

  [[nodiscard]] constexpr decltype(auto) topElement() {
    return std::integral_constant<BinaryDomain, BinaryDomain::TOP>{};
  }

  [[nodiscard]] constexpr decltype(auto) bottomElement() {
    return std::integral_constant<BinaryDomain, BinaryDomain::BOTTOM>{};
  }

  [[nodiscard]] constexpr l_t join(auto L, auto R) {
    if (L != R) {
      return bottomElement();
    }
    return L;
  }

  // --- IsSemiRing:

  [[nodiscard]] constexpr decltype(auto)
  extend(IsEdgeFunctionFor<l_t> auto && /*First*/,
         IsEdgeFunctionFor<l_t> auto && /*Second*/) {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr decltype(auto)
  combine(IsEdgeFunctionFor<l_t> auto && /*First*/,
          IsEdgeFunctionFor<l_t> auto && /*Second*/) {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr decltype(auto) identity() {
    return EdgeFunctionType{};
  }
};

template <IFDSProblem ProblemTy>
IfdsToIdeProblemWrapper(ProblemTy *)
    -> IfdsToIdeProblemWrapper<IDEProblemWrapper<ProblemTy>>;

template <IFDSProblem ProblemTy>
IfdsToIdeProblemWrapper(NonNullPtr<ProblemTy>)
    -> IfdsToIdeProblemWrapper<IDEProblemWrapper<ProblemTy>>;

} // namespace psr
