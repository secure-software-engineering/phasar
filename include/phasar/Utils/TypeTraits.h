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

#include <ostream>
#include <tuple>
#include <type_traits>

#include "llvm/Support/raw_ostream.h"

namespace psr {
namespace detail {

template <typename T, typename = void>
struct is_iterable : public std::false_type {};
template <typename T>
struct is_iterable<T, std::void_t<typename T::const_iterator,
                                  decltype(std::declval<T>().begin()),
                                  decltype(std::declval<T>().end())>>
    : public std::true_type {};

template <typename T> struct is_pair : public std::false_type {};
template <typename U, typename V>
struct is_pair<std::pair<U, V>> : public std::true_type {};

template <typename T> struct is_tuple : public std::false_type {};
template <typename... Elems>
struct is_tuple<std::tuple<Elems...>> : public std::true_type {};

template <typename T, typename OS, typename = OS &>
struct is_printable : public std::false_type {};
template <typename T, typename OS>
struct is_printable<T, OS, decltype(std::declval<OS &>() << std::declval<T>())>
    : public std::true_type {};

template <typename T>
using is_llvm_printable = is_printable<T, llvm::raw_ostream>;

template <typename T> using is_std_printable = is_printable<T, std::ostream>;

template <typename T, typename Enable = std::string>
struct has_str : public std::false_type {};
template <typename T>
struct has_str<T, decltype(std::declval<T>().str())> : public std::true_type {};

template <typename T, typename = void>
struct has_erase_iterator : std::false_type {};
template <typename T>
struct has_erase_iterator<T, std::void_t<decltype(std::declval<T>().erase(
                                 std::declval<typename T::iterator>()))>>
    : std::true_type {};

template <typename T, typename = size_t>
struct is_std_hashable : std::false_type {};
template <typename T>
struct is_std_hashable<T, decltype(std::declval<std::hash<T>>()(
                              std::declval<T>()))> : std::true_type {};

template <typename T, typename = llvm::hash_code>
struct is_llvm_hashable : std::false_type {};
template <typename T>
struct is_llvm_hashable<T, decltype(hash_value(std::declval<T>()))>
    : std::true_type {};

} // namespace detail

template <typename T>
constexpr bool is_iterable_v = detail::is_iterable<T>::value;

template <typename T> constexpr bool is_pair_v = detail::is_pair<T>::value;

template <typename T> constexpr bool is_tuple_v = detail::is_tuple<T>::value;

template <typename T>
constexpr bool is_llvm_printable_v = detail::is_llvm_printable<T>::value;

template <typename T>
constexpr bool is_std_printable_v = detail::is_std_printable<T>::value;

template <typename T, typename OS>
constexpr bool is_printable_v = detail::is_printable<T, OS>::value;

template <typename T> constexpr bool has_str_v = detail::has_str<T>::value;

template <typename T>
constexpr bool has_erase_iterator_v = detail::has_erase_iterator<T>::value;

template <typename T>
constexpr bool is_std_hashable_v = detail::is_std_hashable<T>::value;

template <typename T>
constexpr bool is_llvm_hashable_v = detail::is_llvm_hashable<T>::value;
} // namespace psr

#endif // PHASAR_UTILS_TYPETRAITS_H