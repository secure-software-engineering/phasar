/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_NULLABLE_H
#define PHASAR_UTILS_NULLABLE_H

#include <cassert>
#include <optional>
#include <type_traits>
#include <utility>

namespace psr {

template <typename T>
using Nullable =
    std::conditional_t<std::is_convertible_v<T, bool>, T, std::optional<T>>;

template <typename T>
  requires std::is_convertible_v<T, bool>
[[nodiscard]] constexpr T unwrapNullable(T &&Val) noexcept {
  return std::forward<T>(Val);
}
template <typename T>
  requires(!std::is_convertible_v<T, bool>)
[[nodiscard]] constexpr T unwrapNullable(std::optional<T> &&Val) noexcept {
  assert(Val && "Unwrapping nullopt!");
  return *std::move(Val);
}
template <typename T>
  requires(!std::is_convertible_v<T, bool>)
[[nodiscard]] constexpr const T &
unwrapNullable(const std::optional<T> &Val) noexcept {
  assert(Val && "Unwrapping nullopt!");
  return *Val;
}
template <typename T>
  requires(!std::is_convertible_v<T, bool>)
[[nodiscard]] constexpr T &unwrapNullable(std::optional<T> &Val) noexcept {
  assert(Val && "Unwrapping nullopt!");
  return *Val;
}
} // namespace psr

#endif // PHASAR_UTILS_NULLABLE_H
