/******************************************************************************
 * Copyright (c) 2021 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#ifndef PHASAR_UTILS_TYPETRAITS_H
#define PHASAR_UTILS_TYPETRAITS_H

#include "llvm/Support/raw_ostream.h"

#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace psr {
// NOLINTBEGIN(readability-identifier-naming)
namespace detail {

template <typename T> struct is_pair : std::false_type {}; // NOLINT
template <typename U, typename V>
struct is_pair<std::pair<U, V>> : std::true_type {}; // NOLINT

template <typename T> struct is_tuple : std::false_type {}; // NOLINT
template <typename... Elems>
struct is_tuple<std::tuple<Elems...>> : std::true_type {}; // NOLINT

template <template <typename> typename Base, typename Derived>
class template_arg {
private:
  template <template <typename> typename TBase, typename TT>
  static TT getTemplateArgImpl(const TBase<TT> &Impl);
  template <template <typename> typename TBase>
  static void getTemplateArgImpl(...);

public:
  using type =
      decltype(getTemplateArgImpl<Base>(std::declval<const Derived &>()));
};

template <template <typename> typename Base, typename Derived, typename = void>
struct is_crtp_base_of : std::false_type {}; // NOLINT
template <template <typename> typename Base, typename Derived>
struct is_crtp_base_of<
    Base, Derived,
    std::enable_if_t<
        std::is_base_of_v<typename template_arg<Base, Derived>::type, Derived>>>
    : std::true_type {};

template <typename T, typename = bool>
struct HasIsConstant : std::false_type {};
template <typename T>
struct HasIsConstant<T, decltype(std::declval<const T &>().isConstant())>
    : std::true_type {};

template <typename T, typename = bool>
struct IsEqualityComparable : std::false_type {};
template <typename T>
struct IsEqualityComparable<T, decltype(std::declval<T>() == std::declval<T>())>
    : std::true_type {};

template <typename T, typename U, typename = bool>
struct AreEqualityComparable : std::false_type {};
template <typename T, typename U>
struct AreEqualityComparable<T, U,
                             decltype(std::declval<T>() == std::declval<U>())>
    : std::true_type {};

} // namespace detail

template <typename T>
constexpr bool is_pair_v = detail::is_pair<T>::value; // NOLINT

template <typename T>
constexpr bool is_tuple_v = detail::is_tuple<T>::value; // NOLINT

template <typename T> struct is_variant : std::false_type {}; // NOLINT

template <typename... Args>
struct is_variant<std::variant<Args...>> : std::true_type {}; // NOLINT

template <typename T>
inline constexpr bool is_variant_v = is_variant<T>::value; // NOLINT

template <template <typename> typename Base, typename Derived>
constexpr bool is_crtp_base_of_v = // NOLINT
    detail::is_crtp_base_of<Base, Derived>::value;

template <typename T>
static inline constexpr bool HasIsConstant = detail::HasIsConstant<T>::value;

template <typename T>
static inline constexpr bool IsEqualityComparable =
    detail::IsEqualityComparable<T>::value;

template <typename T, typename U>
static inline constexpr bool AreEqualityComparable =
    detail::AreEqualityComparable<T, U>::value;

struct TrueFn {
  template <typename... Args>
  [[nodiscard]] bool operator()(const Args &.../*unused*/) const noexcept {
    return true;
  }
};

struct FalseFn {
  template <typename... Args>
  [[nodiscard]] bool operator()(const Args &.../*unused*/) const noexcept {
    return false;
  }
};

struct EmptyType {
  friend constexpr bool operator==(EmptyType /*L*/, EmptyType /*R*/) noexcept {
    return true;
  }
  friend constexpr bool operator!=(EmptyType /*L*/, EmptyType /*R*/) noexcept {
    return false;
  }
};

/// Delegates to the ctor of T
template <typename T> struct DefaultConstruct {
  template <typename... U>
  [[nodiscard]] inline T
  operator()(U &&...Val) noexcept(std::is_nothrow_constructible_v<T, U...>) {
    return T(std::forward<U>(Val)...);
  }
};

// NOLINTEND(readability-identifier-naming)
} // namespace psr

#if __cplusplus < 202002L
#include "phasar/Utils/detail/CXX17TypeTraits.h"
#else
#include "phasar/Utils/detail/CXX20TypeTraits.h"
#endif

#endif
