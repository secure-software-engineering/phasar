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

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
class EnumFlagAutoBool {
  T Val;

public:
  constexpr EnumFlagAutoBool(T Val) : Val(Val) {}
  constexpr operator T() const { return Val; }
  constexpr explicit operator bool() const {
    return static_cast<std::underlying_type_t<T>>(Val) != 0;
  }
};

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr EnumFlagAutoBool<T> operator&(T Lhs, T Rhs) {
  return static_cast<T>(static_cast<typename std::underlying_type_t<T>>(Lhs) &
                        static_cast<typename std::underlying_type_t<T>>(Rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr void operator&=(T &Lhs, T Rhs) {
  Lhs = static_cast<T>(static_cast<typename std::underlying_type_t<T>>(Lhs) &
                       static_cast<typename std::underlying_type_t<T>>(Rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr T operator|(T Lhs, T Rhs) {
  return static_cast<T>(static_cast<typename std::underlying_type_t<T>>(Lhs) |
                        static_cast<typename std::underlying_type_t<T>>(Rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr void operator|=(T &Lhs, T Rhs) {
  Lhs = static_cast<T>(static_cast<typename std::underlying_type_t<T>>(Lhs) |
                       static_cast<typename std::underlying_type_t<T>>(Rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr T operator^(T Lhs, T Rhs) {
  return static_cast<T>(static_cast<typename std::underlying_type_t<T>>(Lhs) ^
                        static_cast<typename std::underlying_type_t<T>>(Rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr void operator^=(T &Lhs, T Rhs) {
  Lhs = static_cast<T>(static_cast<typename std::underlying_type_t<T>>(Lhs) ^
                       static_cast<typename std::underlying_type_t<T>>(Rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr T operator~(T Rhs) {
  return static_cast<T>(~static_cast<typename std::underlying_type_t<T>>(Rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr void setFlag(T &This, T Flag, bool Set = true) {
  if (Set) {
    This |= Flag;
  } else {
    This &= ~Flag;
  }
}
template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr bool hasFlag(T This, T Flag) {
  return static_cast<bool>(This & Flag);
}
} // namespace psr

#endif
