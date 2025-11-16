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

#include "phasar/Utils/Macros.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <concepts>
#include <iterator>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace psr {

using std::type_identity;
using std::type_identity_t;

/// \file TODO: We should stick to one naming convention here and not mix
/// CamelCase with lower_case!

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

template <typename Var, typename T> struct variant_idx;
template <typename... Ts, typename T>
struct variant_idx<std::variant<Ts...>, T>
    : std::integral_constant<
          size_t,
          std::variant<type_identity<Ts>...>(type_identity<T>{}).index()> {};

template <typename Container> struct ElementType {
  using IteratorTy =
      std::decay_t<decltype(llvm::adl_begin(std::declval<Container>()))>;
  using type = typename std::iterator_traits<IteratorTy>::value_type;
};

} // namespace detail
template <typename T, typename U>
concept same_as_decay =
    std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <typename T, typename U>
concept derived_from_decay =
    std::derived_from<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <typename T>
concept is_iterable_v = requires(T &Val) {
  std::begin(Val);
  std::end(Val);
} || requires(T &Val) {
  begin(Val);
  end(Val);
};

template <typename T, typename Over>
concept is_iterable_over_v = is_iterable_v<T> && requires(T &Val) {
  { *llvm::adl_begin(Val) } -> same_as_decay<Over>;
};

template <typename T>
concept is_pair_v = detail::is_pair<T>::value; // NOLINT

template <typename T>
concept is_tuple_v = detail::is_tuple<T>::value; // NOLINT

template <typename T, typename OS>
concept is_printable_v = requires(const T &Val, OS &Stream) { Stream << Val; };

template <typename T>
concept is_llvm_printable_v = is_printable_v<T, llvm::raw_ostream>;

template <typename T>
concept is_std_printable_v = is_printable_v<T, std::ostream>;

template <typename T>
concept has_str_v = requires(const T &Val) {
  { Val.str() } -> same_as_decay<std::string>;
};

template <typename T>
concept has_adl_to_string_v = requires(const T &Val) {
  { std::to_string(Val) } -> std::convertible_to<std::string_view>;
} || requires(const T &Val) {
  { to_string(Val) } -> std::convertible_to<std::string_view>;
};

template <typename T>
concept has_erase_iterator_v = requires(
    T &Val, typename std::remove_cvref_t<T>::iterator It) { Val.erase(It); };

template <typename T>
concept is_std_hashable_v = requires(const T &Val) {
  { std::hash<T>{}(Val) } -> std::convertible_to<size_t>;
};

template <typename T>
concept is_llvm_hashable_v = requires(const T &Val) {
  { llvm::hash_value(Val) } -> std::convertible_to<size_t>;
} || requires(const T &Val) {
  { hash_value(Val) } -> std::convertible_to<size_t>;
};

template <typename T> struct is_variant : std::false_type {}; // NOLINT

template <typename... Args>
struct is_variant<std::variant<Args...>> : std::true_type {}; // NOLINT

template <typename T>
concept is_variant_v = is_variant<T>::value; // NOLINT

template <typename T>
// NOLINTNEXTLINE
concept is_string_like_v = std::is_convertible_v<T, std::string_view>;

template <template <typename> typename Base, typename Derived>
concept is_crtp_base_of_v =
    std::is_base_of_v<typename detail::template_arg<Base, Derived>::type,
                      Derived>;

template <typename T>
concept HasIsConstant = requires(const T &Val) {
  { Val.isConstant() } -> std::convertible_to<bool>;
};

template <typename T>
concept HasDepth = requires(const T &Val) {
  { Val.depth() } -> std::convertible_to<bool>;
};

template <typename T>
concept IsEqualityComparable = requires(const T &Val) {
  { Val == Val } -> std::convertible_to<bool>;
};

template <typename T, typename U>
concept AreEqualityComparable = requires(const T &Val1, const U &Val2) {
  { Val1 == Val2 } -> std::convertible_to<bool>;
};

template <typename T>
concept IsLessComparable = requires(const T &Val1, const T &Val2) {
  { Val1 < Val2 } -> std::convertible_to<bool>;
};

template <typename ProblemTy>
concept has_isInteresting_v = requires(
    const ProblemTy &Val, typename ProblemTy::ProblemAnalysisDomain::n_t Nod) {
  { Val.isInteresting(Nod) } -> std::convertible_to<bool>;
};

template <typename T>
constexpr bool has_llvm_dense_map_info = requires(const T &Val) {
  { llvm::DenseMapInfo<T>::getEmptyKey() } -> std::same_as<T>;
  { llvm::DenseMapInfo<T>::getTombstoneKey() } -> std::same_as<T>;
  { llvm::DenseMapInfo<T>::getHashValue(Val) } -> std::convertible_to<size_t>;
  { llvm::DenseMapInfo<T>::isEqual(Val, Val) } -> std::convertible_to<bool>;
};

template <typename From, typename To>
concept is_explicitly_convertible_to =
    requires(From F) { static_cast<To>(std::forward<From>(F)); };

template <typename Var, typename T>
constexpr size_t variant_idx = detail::variant_idx<Var, T>::value;

template <typename Container>
using ElementType = typename detail::ElementType<Container>::type;
template <typename T, typename Enable = void>
struct [[deprecated("getAsJson should not be used anymore. Use printAsJson "
                    "instead")]] has_getAsJson : std::false_type {}; // NOLINT
template <typename T>
struct [[deprecated(
    "getAsJson should not be used anymore. Use printAsJson "
    "instead")]] has_getAsJson<T, std::void_t<decltype(std::declval<const T>()
                                                           .getAsJson())>>
    : std::true_type {}; // NOLINT

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

template <typename T>
concept has_reserve_v = requires(T &Val) { Val.reserve(size_t(0)); };

template <typename T> void reserveIfPossible(T &Container, size_t Capacity) {
  if constexpr (has_reserve_v<T>) {
    Container.reserve(Capacity);
  }
}

template <has_adl_to_string_v T>
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
