/******************************************************************************
 * Copyright (c) 2021 Fabian Schiebel.
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
#include "phasar/Utils/TypeTraits.h"

namespace psr {
namespace detail {

template <typename OS_t, typename T> void printHelper(OS_t &OS, const T &Data);

template <typename OS_t, typename... Args, size_t... Idx>
void printTuple(OS_t &OS, const std::tuple<Args...> &Tup,
                std::index_sequence<Idx...> /*unused*/) {
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
  } else if constexpr (is_printable_v<ElemTy, OS_t>) {
    OS << Data;
  } else if constexpr (is_pair_v<ElemTy>) {
    OS << "(";
    printHelper(OS, Data.first);
    OS << ", ";
    printHelper(OS, Data.second);
    OS << ")";
  } else if constexpr (is_tuple_v<ElemTy>) {
    printTuple(OS, Data, std::make_index_sequence<std::tuple_size_v<ElemTy>>());
  } else if constexpr (has_str_v<ElemTy>) {
    OS << Data.str();
  } else if constexpr (is_iterable_v<ElemTy>) {
    OS << "{ ";
    bool Frst = true;
    size_t Cnt = 0;
    for (auto &&Elem : Data) {
      ++Cnt;
      if (Frst) {
        Frst = false;
      } else {
        OS << ", ";
      }

      printHelper(OS, Elem);
    }

    OS << (Frst ? "}" : " }");
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

#endif
