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
                        ByConstRef<d_t> /*SuccNode*/) noexcept {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr decltype(auto)
  getCallEdgeFunction(ByConstRef<n_t> /*CallSite*/, ByConstRef<d_t> /*CSNode*/,
                      ByConstRef<f_t> /*CalleeFun*/,
                      ByConstRef<d_t> /*CalleeNode*/) noexcept {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr decltype(auto) getReturnEdgeFunction(
      ByConstRef<n_t> /*CallSite*/, ByConstRef<f_t> /*CalleeFun*/,
      ByConstRef<n_t> /*ExitInst*/, ByConstRef<d_t> /*ExitNode*/,
      ByConstRef<n_t> /*RetSite*/, ByConstRef<d_t> /*RSNode*/) noexcept {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr decltype(auto) getCallToRetEdgeFunction(
      ByConstRef<n_t> /*CallSite*/, ByConstRef<d_t> /*CSNode*/,
      ByConstRef<n_t> /*RetSite*/, ByConstRef<d_t> /*RSNode*/,
      llvm::ArrayRef<f_t> /*Callees*/) noexcept {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr decltype(auto)
  getSummaryEdgeFunction(ByConstRef<n_t> /*Curr*/, ByConstRef<d_t> /*CurrNode*/,
                         ByConstRef<n_t> /*Succ*/,
                         ByConstRef<d_t> /*SuccNode*/) noexcept {
    return EdgeFunctionType{};
  }

  // --- IsJoinLattice:

  [[nodiscard]] constexpr auto topElement() noexcept {
    return std::integral_constant<BinaryDomain, BinaryDomain::TOP>{};
  }

  [[nodiscard]] constexpr auto bottomElement() noexcept {
    return std::integral_constant<BinaryDomain, BinaryDomain::BOTTOM>{};
  }

  [[nodiscard]] constexpr l_t join(l_t L, l_t R) noexcept {
    if (L != R) {
      return bottomElement();
    }
    return L;
  }

  // --- IsSemiRing:

  [[nodiscard]] constexpr auto extend(EdgeFunctionType /*First*/,
                                      EdgeFunctionType /*Second*/) noexcept {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr auto combine(EdgeFunctionType /*First*/,
                                       EdgeFunctionType /*Second*/) noexcept {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr auto identity() noexcept {
    return EdgeFunctionType{};
  }

  [[nodiscard]] constexpr auto allTopFunction() noexcept {
    // technically not 100% correct, but should work in practice
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
