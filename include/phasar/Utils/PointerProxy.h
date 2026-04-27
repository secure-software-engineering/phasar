#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/Macros.h"

namespace psr {
template <typename T> struct PointerProxy {
  T Value;

  [[nodiscard]] constexpr T *get() noexcept PSR_LIFETIMEBOUND { return &Value; }
  [[nodiscard]] constexpr T *operator->() noexcept PSR_LIFETIMEBOUND {
    return &Value;
  }

  [[nodiscard]] constexpr T &operator*() noexcept PSR_LIFETIMEBOUND {
    return Value;
  }
};
} // namespace psr
