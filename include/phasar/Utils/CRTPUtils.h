/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_CRTPUTILS_H
#define PHASAR_UTILS_CRTPUTILS_H

namespace psr {
template <typename Derived> class CRTPBase {
  friend Derived;

protected:
  constexpr CRTPBase() noexcept = default;

  [[nodiscard]] constexpr Derived &self() & noexcept {
    return static_cast<Derived &>(*this);
  }
  [[nodiscard]] constexpr Derived &&self() && noexcept {
    return static_cast<Derived &&>(*this);
  }
  [[nodiscard]] constexpr const Derived &self() const & noexcept {
    return static_cast<const Derived &>(*this);
  }
};
} // namespace psr

#endif // PHASAR_UTILS_CRTPUTILS_H
