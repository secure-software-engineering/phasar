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

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace psr {

#if __cplusplus < 202002L
#define PSR_CONCEPT static constexpr bool
template <typename T> struct type_identity { using type = T; };
#else
#define PSR_CONCEPT concept
template <typename T> using type_identity = std::type_identity<T>;
#endif

// NOLINTBEGIN(readability-identifier-naming)
namespace detail {

template <typename T, typename = void>
struct is_iterable : std::false_type {}; // NOLINT
template <typename T>
struct is_iterable<T, std::void_t<decltype(llvm::adl_begin(std::declval<T>())),
                                  decltype(llvm::adl_end(std::declval<T>()))>>
    : public std::true_type {};

template <typename T, typename U, typename = void>
struct is_iterable_over : std::false_type {}; // NOLINT
template <typename T, typename U>
struct is_iterable_over<
    T, U,
    std::enable_if_t<is_iterable<T>::value &&
                     std::is_same_v<U, std::decay_t<decltype(*llvm::adl_begin(
                                           std::declval<T>()))>>>>
    : public std::true_type {};

template <typename T> struct is_pair : std::false_type {}; // NOLINT
template <typename U, typename V>
struct is_pair<std::pair<U, V>> : std::true_type {}; // NOLINT

template <typename T> struct is_tuple : std::false_type {}; // NOLINT
template <typename... Elems>
struct is_tuple<std::tuple<Elems...>> : std::true_type {}; // NOLINT

template <typename T, typename OS, typename = OS &>
struct is_printable : std::false_type {}; // NOLINT
template <typename T, typename OS>
struct is_printable< // NOLINT
    T, OS, decltype(std::declval<OS &>() << std::declval<T>())>
    : std::true_type {};

template <typename T>
using is_llvm_printable = is_printable<T, llvm::raw_ostream>; // NOLINT

template <typename T>
using is_std_printable = is_printable<T, std::ostream>; // NOLINT

template <typename T, typename Enable = std::string>
struct has_str : std::false_type {}; // NOLINT
template <typename T>
struct has_str<T, decltype(std::declval<T>().str())> : std::true_type {
}; // NOLINT

template <typename T, typename = void> struct has_reserve : std::false_type {};
template <typename T>
struct has_reserve<
    T, std::void_t<decltype(std::declval<T &>().reserve(size_t(0)))>>
    : std::true_type {};

template <typename T> struct has_adl_to_string {
  template <typename TT = T, typename = decltype(std::string_view(
                                 to_string(std::declval<TT>())))>
  static std::true_type test(int);
  template <typename TT = T,
            typename = decltype(std::to_string(std::declval<TT>()))>
  static std::true_type test(long);
  template <typename TT = T> static std::false_type test(...);

  static constexpr bool value =
      std::is_same_v<std::true_type, decltype(test(0))>;
};

template <typename T, typename = void>
struct has_erase_iterator : std::false_type {}; // NOLINT
template <typename T>
struct has_erase_iterator< // NOLINT
    T, std::void_t<decltype(std::declval<T>().erase(
           std::declval<typename T::iterator>()))>> : std::true_type {};

template <typename T, typename = size_t>
struct is_std_hashable : std::false_type {}; // NOLINT
template <typename T>
struct is_std_hashable<T, decltype(std::declval<std::hash<T>>()( // NOLINT
                              std::declval<T>()))> : std::true_type {};

template <typename T, typename = llvm::hash_code>
struct is_llvm_hashable : std::false_type {}; // NOLINT
template <typename T>
struct is_llvm_hashable<T, decltype(hash_value(std::declval<T>()))> // NOLINT
    : std::true_type {};
template <typename T>
struct is_llvm_hashable<T,
                        decltype(llvm::hash_value(std::declval<T>()))> // NOLINT
    : std::true_type {};

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

template <typename T, typename = size_t> struct HasDepth : std::false_type {};
template <typename T>
struct HasDepth<T, decltype(std::declval<const T &>().depth())>
    : std::true_type {};

template <typename Var, typename T> struct variant_idx;
template <typename... Ts, typename T>
struct variant_idx<std::variant<Ts...>, T>
    : std::integral_constant<
          size_t,
          std::variant<type_identity<Ts>...>(type_identity<T>{}).index()> {};

template <typename ProblemTy, typename = bool>
struct has_isInteresting : std::false_type {}; // NOLINT
template <typename ProblemTy>
struct has_isInteresting<
    ProblemTy,
    decltype(std::declval<std::add_const_t<ProblemTy>>().isInteresting(
        std::declval<typename ProblemTy::ProblemAnalysisDomain::n_t>()))>
    : std::true_type {};

template <typename T, typename = void>
struct has_llvm_dense_map_info : std::false_type {};
template <typename T>
struct has_llvm_dense_map_info<
    T, std::void_t<decltype(llvm::DenseMapInfo<T>::getEmptyKey()),
                   decltype(llvm::DenseMapInfo<T>::getTombstoneKey()),
                   decltype(llvm::DenseMapInfo<T>::getHashValue(
                       std::declval<T>())),
                   decltype(llvm::DenseMapInfo<T>::isEqual(std::declval<T>(),
                                                           std::declval<T>()))>>
    : std::true_type {};
} // namespace detail

template <typename T>
PSR_CONCEPT is_iterable_v = detail::is_iterable<T>::value; // NOLINT

template <typename T, typename Over>
PSR_CONCEPT is_iterable_over_v = // NOLINT
    detail::is_iterable_over<T, Over>::value;

template <typename T>
PSR_CONCEPT is_pair_v = detail::is_pair<T>::value; // NOLINT

template <typename T>
PSR_CONCEPT is_tuple_v = detail::is_tuple<T>::value; // NOLINT

template <typename T>
PSR_CONCEPT is_llvm_printable_v = // NOLINT
    detail::is_llvm_printable<T>::value;

template <typename T>
PSR_CONCEPT is_std_printable_v = // NOLINT
    detail::is_std_printable<T>::value;

template <typename T, typename OS>
PSR_CONCEPT is_printable_v = detail::is_printable<T, OS>::value; // NOLINT

template <typename T>
PSR_CONCEPT has_str_v = detail::has_str<T>::value; // NOLINT

template <typename T>
PSR_CONCEPT has_adl_to_string_v = detail::has_adl_to_string<T>::value;

template <typename T>
PSR_CONCEPT has_erase_iterator_v = // NOLINT
    detail::has_erase_iterator<T>::value;

template <typename T>
PSR_CONCEPT is_std_hashable_v = detail::is_std_hashable<T>::value; // NOLINT

template <typename T>
PSR_CONCEPT is_llvm_hashable_v = // NOLINT
    detail::is_llvm_hashable<T>::value;

template <typename T> struct is_variant : std::false_type {}; // NOLINT

template <typename... Args>
struct is_variant<std::variant<Args...>> : std::true_type {}; // NOLINT

template <typename T> PSR_CONCEPT is_variant_v = is_variant<T>::value; // NOLINT

template <typename T>
// NOLINTNEXTLINE
PSR_CONCEPT is_string_like_v = std::is_convertible_v<T, std::string_view>;

template <template <typename> typename Base, typename Derived>
PSR_CONCEPT is_crtp_base_of_v = // NOLINT
    detail::is_crtp_base_of<Base, Derived>::value;

template <typename T>
PSR_CONCEPT HasIsConstant = detail::HasIsConstant<T>::value;

template <typename T> PSR_CONCEPT HasDepth = detail::HasDepth<T>::value;

template <typename T>
PSR_CONCEPT IsEqualityComparable = detail::IsEqualityComparable<T>::value;

template <typename T, typename U>
PSR_CONCEPT AreEqualityComparable = detail::AreEqualityComparable<T, U>::value;

template <typename ProblemTy>
PSR_CONCEPT has_isInteresting_v = // NOLINT
    detail::has_isInteresting<ProblemTy>::value;

template <typename T>
static constexpr bool has_llvm_dense_map_info =
    detail::has_llvm_dense_map_info<T>::value;
template <typename T> using type_identity_t = typename type_identity<T>::type;

template <typename Var, typename T>
static constexpr size_t variant_idx = detail::variant_idx<Var, T>::value;

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

/// Delegates to the ctor of T
template <typename T> struct DefaultConstruct {
  template <typename... U>
  [[nodiscard]] inline T
  operator()(U &&...Val) noexcept(std::is_nothrow_constructible_v<T, U...>) {
    return T(std::forward<U>(Val)...);
  }
};

struct IgnoreArgs {
  template <typename... U> void operator()(U &&.../*Val*/) noexcept {}
};

template <typename T> void reserveIfPossible(T &Container, size_t Capacity) {
  if constexpr (detail::has_reserve<T>::value) {
    Container.reserve(Capacity);
  }
}

template <typename T, typename = std::enable_if_t<has_adl_to_string_v<T>>>
[[nodiscard]] decltype(auto) adl_to_string(const T &Val) {
  using std::to_string;
  return to_string(Val);
}

struct IdentityFn {
  template <typename T> decltype(auto) operator()(T &&Val) const noexcept {
    return std::forward<decltype(Val)>(Val);
  }
};

// NOLINTEND(readability-identifier-naming)
} // namespace psr

#endif
