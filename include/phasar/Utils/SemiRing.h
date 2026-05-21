/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#pragma once

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/Domain/BinaryDomain.h"

#include <concepts>

namespace psr {

template <typename T>
concept IsSemiRing = requires(T &SR, const typename T::EdgeFunctionType &EF) {
  typename T::EdgeFunctionType;
  requires IsEdgeFunction<typename T::EdgeFunctionType>;

  { SR.extend(EF, EF) } -> std::convertible_to<typename T::EdgeFunctionType>;
  { SR.combine(EF, EF) } -> std::convertible_to<typename T::EdgeFunctionType>;
  { SR.identity() } -> std::convertible_to<typename T::EdgeFunctionType>;
};
template <typename T, typename L>
concept IsSemiRingOf = IsSemiRing<T> && requires {
  requires std::same_as<typename T::EdgeFunctionType::l_t, L>;
};

template <typename AnalysisDomainTy> class SemiRing {
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

  virtual EdgeFunctionType identity() const { return EdgeIdentity<l_t>{}; }
};

struct BinarySemiRing {
  using EdgeFunctionType = EdgeIdentity<BinaryDomain>;

  [[nodiscard]] constexpr EdgeFunctionType
  extend(EdgeFunctionType /*L*/, EdgeFunctionType /*R*/) const noexcept {
    return {};
  }

  [[nodiscard]] constexpr EdgeFunctionType
  combine(EdgeFunctionType /*L*/, EdgeFunctionType /*R*/) const noexcept {
    return {};
  }

  [[nodiscard]] constexpr EdgeFunctionType identity() const noexcept {
    return {};
  }

  static constinit BinarySemiRing Instance;
};

inline constinit BinarySemiRing BinarySemiRing::Instance{};
} // namespace psr
