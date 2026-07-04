#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <functional>
#include <type_traits>
#include <utility>

namespace psr {
// NOLINTNEXTLINE(readability-identifier-naming)
template <typename Fn> struct lazy {
  Fn F;

  template <typename FF>
    requires(!std::is_same_v<std::remove_cvref_t<FF>, lazy>)
  lazy(FF &&F) noexcept(std::is_nothrow_constructible_v<Fn, FF &&>)
      : F(std::forward<FF>(F)) {}

  constexpr operator std::invoke_result_t<Fn>() && noexcept(
      std::is_nothrow_invocable_v<Fn>) {
    return std::invoke(std::move(F));
  }
};

template <typename FF> lazy(FF) -> lazy<std::decay_t<FF>>;

#define PSR_LAZY(...)                                                          \
  ::psr::lazy {                                                                \
    [&] { return __VA_ARGS__; }                                                \
  }
} // namespace psr
