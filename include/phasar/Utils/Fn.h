/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_FN_H
#define PHASAR_UTILS_FN_H

#include "phasar/Utils/Macros.h"

#include <functional>
#include <type_traits>

namespace psr {
/// \brief A helper that transforms a statically known function pointer into a
/// type.
///
/// Useful for passing functions as callbacks, while avoiding either the
/// indirect call overhead or wrapping a lambda around
template <auto F> struct fn_t { // NOLINT(readability-identifier-naming)
  template <typename... ArgsT>
  constexpr std::invoke_result_t<decltype(F), ArgsT...>
  operator()(ArgsT &&...Args) const
      noexcept(std::is_nothrow_invocable_v<decltype(F), ArgsT...>) {
    return std::invoke(F, PSR_FWD(Args)...);
  }
};

/// \brief A helper that transforms a statically known function pointer into a
/// callable object.
///
/// Useful for passing functions as callbacks, while avoiding either the
/// indirect call overhead or wrapping a lambda around
template <auto F>
static constexpr fn_t<F> fn{}; // NOLINT(readability-identifier-naming)

} // namespace psr

#endif // PHASAR_UTILS_FN_H
