#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Utils/ValueCompressor.h"

namespace psr {

/// Base interface for results of a ValueId-based alias-analysis.
template <typename T>
concept RawAAResult = requires(const T &Result, ValueId Var) {
  { T::isCached() } noexcept -> std::convertible_to<bool>;
  { Result.getRawAliasSet(Var) } -> std::convertible_to<RawAliasSet<ValueId>>;
  { Result.mayAlias(Var, Var) } -> std::convertible_to<bool>;
  { Result.size() } noexcept -> std::convertible_to<size_t>;
};

/// The intersection of two (different) independent raw alias-analysis results.
/// Use this to achieve better precision than feasible with a single analysis.
template <RawAAResult FirstT, RawAAResult SecondT>
struct RawAAResultIntersection {
  [[nodiscard]] static constexpr bool isCached() noexcept {
    // The set-intersection is not cached
    return false;
  }

  [[nodiscard]] constexpr size_t size() const noexcept {
    assert(
        First.size() == Second.size() &&
        "Only alias-results on the same ValueCompressor should be intersected");
    return First.size();
  }

  [[nodiscard]] RawAliasSet<ValueId> getRawAliasSet(ValueId Var) const {
    auto ResultSet = First.getRawAliasSet(Var);
    ResultSet &= Second.getRawAliasSet(Var);
    return ResultSet;
  }

  [[nodiscard]] constexpr bool mayAlias(ValueId Var1, ValueId Var2) const {
    return First.mayAlias(Var1, Var2) && Second.mayAlias(Var1, Var2);
  }

  [[no_unique_address]] FirstT First;
  [[no_unique_address]] SecondT Second;
};
} // namespace psr
