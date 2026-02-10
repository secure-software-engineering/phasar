/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_ENUMFLAGS_H_
#define PHASAR_UTILS_ENUMFLAGS_H_

#include <type_traits>

namespace psr {

namespace detail {
template <typename T>
concept is_enum = std::is_enum_v<T>;
} // namespace detail

template <detail::is_enum T> struct EnumFlagAutoBool {
  T Value;

  constexpr operator T() const noexcept { return Value; }
  constexpr explicit operator bool() const noexcept {
    return static_cast<std::underlying_type_t<T>>(Value) != 0;
  }
};

template <detail::is_enum T>
constexpr EnumFlagAutoBool<T> operator&(T Lhs, T Rhs) noexcept {
  return {static_cast<T>(static_cast<std::underlying_type_t<T>>(Lhs) &
                         static_cast<std::underlying_type_t<T>>(Rhs))};
}

template <detail::is_enum T> constexpr void operator&=(T &Lhs, T Rhs) noexcept {
  Lhs = Lhs & Rhs;
}

template <detail::is_enum T> constexpr T operator|(T Lhs, T Rhs) noexcept {
  return static_cast<T>(static_cast<std::underlying_type_t<T>>(Lhs) |
                        static_cast<std::underlying_type_t<T>>(Rhs));
}

template <detail::is_enum T> constexpr void operator|=(T &Lhs, T Rhs) noexcept {
  Lhs = Lhs | Rhs;
}

template <detail::is_enum T> constexpr T operator^(T Lhs, T Rhs) noexcept {
  return static_cast<T>(static_cast<std::underlying_type_t<T>>(Lhs) ^
                        static_cast<std::underlying_type_t<T>>(Rhs));
}

template <detail::is_enum T> constexpr void operator^=(T &Lhs, T Rhs) noexcept {
  Lhs = Lhs ^ Rhs;
}

template <detail::is_enum T> constexpr T operator~(T Rhs) noexcept {
  return static_cast<T>(~static_cast<std::underlying_type_t<T>>(Rhs));
}

template <detail::is_enum T> constexpr void setFlag(T &This, T Flag) noexcept {
  This |= Flag;
}
template <detail::is_enum T>
constexpr void unsetFlag(T &This, T Flag) noexcept {
  This &= ~Flag;
}
template <detail::is_enum T>
constexpr void setFlag(T &This, T Flag, bool Set) noexcept {
  if (Set) {
    setFlag(This, Flag);
  } else {
    unsetFlag(This, Flag);
  }
}

template <detail::is_enum T> constexpr bool hasFlag(T This, T Flag) noexcept {
  return (This & Flag) == Flag;
}
} // namespace psr

#endif
