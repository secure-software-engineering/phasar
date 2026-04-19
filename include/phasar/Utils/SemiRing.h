/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_SEMIRING_H
#define PHASAR_UTILS_SEMIRING_H

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/Utils/JoinLattice.h"

#include <concepts>

namespace psr {

template <typename AnalysisDomainTy> class AllTopFnProvider {
public:
  virtual ~AllTopFnProvider() = default;
  /// Returns an edge function that represents the top element of the analysis.
  virtual EdgeFunction<typename AnalysisDomainTy::l_t> allTopFunction() = 0;
};

template <typename AnalysisDomainTy>
  requires HasJoinLatticeTraits<typename AnalysisDomainTy::l_t>
class AllTopFnProvider<AnalysisDomainTy> {
public:
  virtual ~AllTopFnProvider() = default;
  /// Returns an edge function that represents the top element of the analysis.
  virtual EdgeFunction<typename AnalysisDomainTy::l_t> allTopFunction() {
    return AllTop<typename AnalysisDomainTy::l_t>{};
  }
};

template <typename AnalysisDomainTy>
class SemiRing : public AllTopFnProvider<AnalysisDomainTy> {
public:
  using l_t = typename AnalysisDomainTy::l_t;
  using EdgeFunctionType = EdgeFunction<l_t>;

  virtual ~SemiRing() = default;

  virtual EdgeFunction<l_t> extend(const EdgeFunction<l_t> &L,
                                   const EdgeFunction<l_t> &R) {
    return L.composeWith(R);
  }

  virtual EdgeFunction<l_t> combine(const EdgeFunction<l_t> &L,
                                    const EdgeFunction<l_t> &R) {
    return L.joinWith(R);
  }

  virtual EdgeFunction<l_t> identity() { return EdgeIdentity<l_t>{}; }
};

template <typename T>
concept IsSemiRing = requires(T &SR, const typename T::EdgeFunctionType &CEF) {
  typename T::EdgeFunctionType;
  requires IsEdgeFunction<typename T::EdgeFunctionType>;

  { SR.extend(CEF, CEF) } -> std::convertible_to<typename T::EdgeFunctionType>;

  { SR.combine(CEF, CEF) } -> std::convertible_to<typename T::EdgeFunctionType>;

  { SR.identity() } -> std::convertible_to<typename T::EdgeFunctionType>;

  { SR.allTopFunction() } -> std::convertible_to<typename T::EdgeFunctionType>;
};

} // namespace psr

#endif // PHASAR_UTILS_SEMIRING_H
