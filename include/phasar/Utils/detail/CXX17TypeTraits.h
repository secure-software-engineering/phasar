#ifndef PHASAR_UTILS_DETAIL_CXX17TYPETRAITS_H
#define PHASAR_UTILS_DETAIL_CXX17TYPETRAITS_H

#include "llvm/Support/raw_ostream.h"

#include <ostream>
#include <string>
#include <type_traits>

namespace psr {
namespace detail {
template <typename T, typename = void>
struct is_iterable : public std::false_type {}; // NOLINT
template <typename T>
struct is_iterable<T, std::void_t< // NOLINT
                          decltype(std::declval<T>().begin()),
                          decltype(std::declval<T>().end())>>
    : public std::true_type {};

template <typename T, typename U, typename = void>
struct is_iterable_over : std::false_type {}; // NOLINT
template <typename T, typename U>
struct is_iterable_over<
    T, U,
    std::enable_if_t<
        is_iterable<T>::value &&
        std::is_convertible_v<decltype(*std::declval<T>().begin()), U>>>
    : std::true_type {};

template <typename T, typename OS, typename = OS &>
struct is_printable : public std::false_type {}; // NOLINT
template <typename T, typename OS>
struct is_printable< // NOLINT
    T, OS, decltype(std::declval<OS &>() << std::declval<T>())>
    : public std::true_type {};

template <typename T>
using is_llvm_printable = is_printable<T, llvm::raw_ostream>; // NOLINT

template <typename T>
using is_std_printable = is_printable<T, std::ostream>; // NOLINT

template <typename T, typename Enable = std::string>
struct has_str : public std::false_type {}; // NOLINT
template <typename T>
struct has_str<T, decltype(std::declval<T>().str())> : public std::true_type {
}; // NOLINT

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

template <typename T> struct type_identity { // NOLINT
  using type = T;
};

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
constexpr bool is_iterable_v = detail::is_iterable<T>::value; // NOLINT

template <typename T, typename Over>
constexpr bool is_iterable_over_v = // NOLINT
    detail::is_iterable_over<T, Over>::value;
template <typename T>
constexpr bool is_llvm_printable_v = // NOLINT
    detail::is_llvm_printable<T>::value;

template <typename T>
constexpr bool is_std_printable_v = // NOLINT
    detail::is_std_printable<T>::value;

template <typename T, typename OS>
constexpr bool is_printable_v = detail::is_printable<T, OS>::value; // NOLINT

template <typename T>
constexpr bool has_str_v = detail::has_str<T>::value; // NOLINT

template <typename T>
constexpr bool has_erase_iterator_v = // NOLINT
    detail::has_erase_iterator<T>::value;

template <typename T>
constexpr bool is_std_hashable_v = detail::is_std_hashable<T>::value; // NOLINT

template <typename T>
constexpr bool is_llvm_hashable_v = // NOLINT
    detail::is_llvm_hashable<T>::value;

template <typename T>
using type_identity_t = typename detail::type_identity<T>::type;

template <typename T>
// NOLINTNEXTLINE
constexpr bool is_string_like_v = std::is_convertible_v<T, std::string_view>;

template <typename T>
static inline constexpr bool HasIsConstant = detail::HasIsConstant<T>::value;

template <typename T>
static inline constexpr bool IsEqualityComparable =
    detail::IsEqualityComparable<T>::value;

template <typename T, typename U>
static inline constexpr bool AreEqualityComparable =
    detail::AreEqualityComparable<T, U>::value;

} // namespace psr

#endif // PHASAR_UTILS_DETAIL_CXX17TYPETRAITS_H
