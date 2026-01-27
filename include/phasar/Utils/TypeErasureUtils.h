/******************************************************************************
 * Copyright (c) 2025 Fabian Schíebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_POINTER_TYPEERASUREUTILS_H
#define PHASAR_POINTER_TYPEERASUREUTILS_H

#include <type_traits>

namespace psr {
struct TypeErasureUtils {
  template <typename ConcreteAA>
  static constexpr bool CanSSO = std::is_trivially_copyable_v<ConcreteAA> &&
                                 sizeof(ConcreteAA) <= sizeof(void *);

  template <typename ConcreteAA>
  constexpr static ConcreteAA *fromOpaquePtr(void *&EF) noexcept {
    if constexpr (CanSSO<ConcreteAA>) {
      return static_cast<ConcreteAA *>(static_cast<void *>(&EF));
    } else {
      return static_cast<ConcreteAA *>(EF);
    }
  }

  template <typename ConcreteAA>
  constexpr static const ConcreteAA *
  fromOpaquePtr(const void *const &EF) noexcept {
    if constexpr (CanSSO<ConcreteAA>) {
      return static_cast<const ConcreteAA *>(static_cast<const void *>(&EF));
    } else {
      return static_cast<const ConcreteAA *>(EF);
    }
  }

  template <typename ConcreteAA>
  constexpr void *getOpaquePtr(ConcreteAA &AA) noexcept {
    if constexpr (CanSSO<ConcreteAA>) {
      void *Ret{};
      ::new (&Ret) ConcreteAA(AA);
      return Ret;
    } else {
      return &AA;
    }
  }

  template <typename ConcreteAA>
  constexpr const void *getOpaquePtr(const ConcreteAA &AA) noexcept {
    if constexpr (CanSSO<ConcreteAA>) {
      void *Ret{};
      ::new (&Ret) ConcreteAA(AA);
      return Ret;
    } else {
      return &AA;
    }
  }
};
} // namespace psr

#endif // PHASAR_POINTER_TYPEERASUREUTILS_H
