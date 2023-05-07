#ifndef PHASAR_UTILS_DETAIL_CXX20TYPETRAITS_H
#define PHASAR_UTILS_DETAIL_CXX20TYPETRAITS_H

#include "llvm/Support/raw_ostream.h"

#include <concepts>
#include <ostream>
#include <string>
#include <utility>

namespace psr {
template <typename T>
concept is_iterable_v = requires(const T &Obj) {
  {Obj.begin()};
  {Obj.end()};
};

template <typename T, typename U>
concept is_iterable_over_v = is_iterable_v<T> &&
    std::convertible_to<decltype(*std::declval<T>().begin()), U>;

template <typename T, typename OS>
concept is_printable_v = requires(const T &Obj, OS &Stream) {
  {Stream << Obj};
};
template <typename T>
concept is_llvm_printable_v = is_printable_v<T, llvm::raw_ostream>;
template <typename T>
concept is_std_printable_v = is_printable_v<T, std::ostream>;

template <typename T>
concept has_str_v = requires(const T &Obj) {
  { Obj.str() } -> std::convertible_to<std::string>;
};

template <typename T>
concept has_erase_iterator_v = requires(T &Obj, typename T::iterator It) {
  { Obj.erase(It); }
};

template <typename T>
concept is_std_hashable_v = requires(const T &Obj) {
  { std::hash<T>{}(Obj) } -> std::convertible_to<size_t>;
};
template <typename T>
concept is_llvm_hashable_v = requires(const T &Obj) {
  { hash_value(Obj) } -> std::convertible_to<llvm::hash_code>;
};

template <typename T> using type_identity_t = std::type_identity_t<T>;

template <typename T>
concept is_string_like_v = std::convertible_to<T, std::string_view>;

template <typename T>
concept HasIsConstant = requires(const T &Obj) {
  { Obj.isConstant() } -> std::same_as<bool>;
};

template <typename T>
concept IsEqualityComparable = std::equality_comparable<T>;
template <typename T, typename U>
AreEqualityComparable = std::equality_comparable_with<T, U>;

} // namespace psr

#endif // PHASAR_UTILS_DETAIL_CXX20TYPETRAITS_H
