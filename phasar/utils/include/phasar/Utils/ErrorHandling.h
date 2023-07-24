/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_ERRORHANDLING_H
#define PHASAR_UTILS_ERRORHANDLING_H

#include "llvm/Support/ErrorOr.h"

#include <functional>
#include <optional>
#include <system_error>
#include <type_traits>

namespace psr {
template <typename T> T getOrThrow(llvm::ErrorOr<T> ValOrErr) {
  if (ValOrErr) {
    return std::move(*ValOrErr);
  }
  throw std::system_error(ValOrErr.getError());
}

template <typename T>
std::optional<T> getOrNull(llvm::ErrorOr<T> ValOrErr) noexcept(
    std::is_nothrow_move_constructible_v<T>) {
  if (ValOrErr) {
    return std::move(*ValOrErr);
  }
  return std::nullopt;
}

template <typename T,
          typename = std::enable_if_t<std::is_default_constructible_v<T>>>
T getOrEmpty(llvm::ErrorOr<T> ValOrErr) noexcept(
    std::is_nothrow_move_constructible_v<T>) {
  if (ValOrErr) {
    return std::move(*ValOrErr);
  }
  return T();
}

template <typename T, typename MapFn>
llvm::ErrorOr<std::invoke_result_t<MapFn, const T &>>
mapValue(const llvm::ErrorOr<T> &ValOrErr,
         MapFn Mapper) noexcept(std::is_nothrow_invocable_v<MapFn, const T &>) {
  if (ValOrErr) {
    return std::invoke(std::move(Mapper), *ValOrErr);
  }
  return ValOrErr.getError();
}

template <typename T, typename MapFn>
llvm::ErrorOr<std::invoke_result_t<MapFn, T>>
mapValue(llvm::ErrorOr<T> &&ValOrErr,
         MapFn Mapper) noexcept(std::is_nothrow_invocable_v<MapFn, T>) {
  if (ValOrErr) {
    return std::invoke(std::move(Mapper), std::move(*ValOrErr));
  }
  return ValOrErr.getError();
}

} // namespace psr

#endif // PHASAR_UTILS_ERRORHANDLING_H
