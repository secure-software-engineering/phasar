/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/
#ifndef PHASAR_UTILS_DEBUGOUTPUT_H
#define PHASAR_UTILS_DEBUGOUTPUT_H

#include <iostream>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <utility>

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/Utils/LLVMShorthands.h"

namespace psr {
namespace detail {

template <typename T, typename = void>
struct is_container : public std::false_type {};
template <typename T>
struct is_container<T, std::void_t<typename T::const_iterator,
                                   decltype(std::declval<T>().begin()),
                                   decltype(std::declval<T>().end())>>
    : public std::true_type {};

template <typename T> struct is_pair : public std::false_type {};
template <typename U, typename V>
struct is_pair<std::pair<U, V>> : public std::true_type {};

template <typename T> struct is_tuple : public std::false_type {};
template <typename... Elems>
struct is_tuple<std::tuple<Elems...>> : public std::true_type {};

template <typename T, typename = llvm::raw_ostream &>
struct is_default_printable : public std::false_type {};
template <typename T>
struct is_default_printable<T, decltype(std::declval<llvm::raw_ostream &>()
                                        << std::declval<T>())>
    : public std::true_type {};

template <typename T, typename Enable = std::string>
struct has_str : public std::false_type {};
template <typename T>
struct has_str<T, decltype(std::declval<T>().str())> : public std::true_type {};

template <typename OS_t, typename T> void printHelper(OS_t &OS, const T &Data);

template <typename OS_t, typename... Args, size_t... Idx>
void printTuple(OS_t &OS, const std::tuple<Args...> &Tup,
                std::index_sequence<Idx...>) {
  OS << "(";
  ((OS << (Idx == 0 ? "" : ", "), printHelper(OS, std::get<Idx>(Tup))), ...);
  OS << ")";
}

template <typename OS_t, typename T> void printHelper(OS_t &OS, const T &Data) {
  using ElemTy = std::decay_t<T>;
  using BaseElemTy = std::decay_t<std::remove_pointer_t<T>>;

  using OSTy = std::decay_t<OS_t>;

  if constexpr (std::is_base_of_v<llvm::Value, BaseElemTy>) {
    OS << llvmIRToString(Data);
  } else if constexpr (std::is_base_of_v<llvm::Type, BaseElemTy>) {
    if constexpr (std::is_base_of_v<llvm::raw_ostream, std::decay_t<OSTy>>) {
      Data->print(OS);
    } else if constexpr (std::is_base_of_v<std::ostream, OSTy>) {
      llvm::raw_os_ostream ROS(OS);
      Data->print(ROS);
    } else {
      std::string Str;
      llvm::raw_string_ostream SOS(Str);
      Data->print(SOS);
      OS << SOS.str();
    }
  } else if constexpr (is_default_printable<ElemTy>::value) {
    OS << Data;
  } else if constexpr (is_pair<ElemTy>::value) {
    OS << "(";
    printHelper(OS, Data.first);
    OS << ", ";
    printHelper(OS, Data.second);
    OS << ")";
  } else if constexpr (is_tuple<ElemTy>::value) {
    printTuple(OS, Data, std::make_index_sequence<std::tuple_size_v<ElemTy>>());
  } else if constexpr (is_container<ElemTy>::value) {
    OS << "{ ";
    bool frst = true;
    size_t Cnt = 0;
    for (auto &&Elem : Data) {
      ++Cnt;
      if (frst)
        frst = false;
      else
        OS << ", ";

      printHelper(OS, Elem);
    }

    OS << (frst ? "}" : " }");
  } else if constexpr (has_str<ElemTy>::value) {
    OS << Data.str();
  } else {
    static_assert(!std::is_same_v<std::decay_t<T>, ElemTy>,
                  "Cannot print elements of the specified type");
  }
}
} // namespace detail

template <typename T> struct PrettyPrinter {
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const PrettyPrinter &P) {
    detail::printHelper(OS, P.Data);
    return OS;
  }

  const T &Data;
};

template <typename T>
std::ostream &operator<<(std::ostream &OS, const PrettyPrinter<T> &P) {
  llvm::raw_os_ostream ROS(OS);
  ROS << P;
  return OS;
}

template <typename T> PrettyPrinter(const T &) -> PrettyPrinter<T>;

} // namespace psr

#endif // PHASAR_UTILS_DEBUGOUTPUT_H_