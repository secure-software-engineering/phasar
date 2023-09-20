/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_DEFAULTVALUE_H
#define PHASAR_UTILS_DEFAULTVALUE_H

#include "phasar/Utils/ByRef.h"

#include <type_traits>

namespace psr {

/// Gets a (cached) reference to the default-constructed value of type T. If T
/// is small and trivially default constructible, creates a temporary instead.
/// Useful for getters that return ByConstRef<T> but need to handle the
/// non-existing-T case
template <typename T,
          typename = std::enable_if_t<std::is_default_constructible_v<T>>>
[[nodiscard]] ByConstRef<T>
getDefaultValue() noexcept(std::is_nothrow_default_constructible_v<T>) {
  auto DefaultConstruct = [] {
    if constexpr (std::is_aggregate_v<T>) {
      return T{};
    } else {
      return T();
    }
  };

  if constexpr (CanEfficientlyPassByValue<T>) {
    return DefaultConstruct();
  } else {
    static T DefaultVal = DefaultConstruct();
    return DefaultVal;
  }
}

} // namespace psr

#endif // PHASAR_UTILS_DEFAULTVALUE_H
