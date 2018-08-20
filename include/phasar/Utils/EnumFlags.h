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
  T val;

public:
  constexpr EnumFlagAutoBool(T val) : val(val) {}
  constexpr operator T() const { return val; }
  constexpr explicit operator bool() const {
    return static_cast<std::underlying_type_t<T>>(val) != 0;
  }
};

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr EnumFlagAutoBool<T> operator&(T lhs, T rhs) {
  return static_cast<T>(static_cast<typename std::underlying_type_t<T>>(lhs) &
                        static_cast<typename std::underlying_type_t<T>>(rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr void operator&=(T &lhs, T rhs) {
  lhs = static_cast<T>(static_cast<typename std::underlying_type_t<T>>(lhs) &
                       static_cast<typename std::underlying_type_t<T>>(rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr T operator|(T lhs, T rhs) {
  return static_cast<T>(static_cast<typename std::underlying_type_t<T>>(lhs) |
                        static_cast<typename std::underlying_type_t<T>>(rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr void operator|=(T &lhs, T rhs) {
  lhs = static_cast<T>(static_cast<typename std::underlying_type_t<T>>(lhs) |
                       static_cast<typename std::underlying_type_t<T>>(rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr T operator^(T lhs, T rhs) {
  return static_cast<T>(static_cast<typename std::underlying_type_t<T>>(lhs) ^
                        static_cast<typename std::underlying_type_t<T>>(rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr void operator^=(T &lhs, T rhs) {
  lhs = static_cast<T>(static_cast<typename std::underlying_type_t<T>>(lhs) ^
                       static_cast<typename std::underlying_type_t<T>>(rhs));
}

template <typename T,
          typename = typename std::enable_if_t<std::is_enum<T>::value, T>>
constexpr T operator~(T t) {
  return static_cast<T>(~static_cast<typename std::underlying_type_t<T>>(t));
}

} // namespace psr

#endif
