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
#include <type_traits>

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/Utils/LLVMShorthands.h"

namespace psr {
namespace detail {
class PrinterBase {
protected:
  const void *value;

public:
  PrinterBase(const void *value) : value(value) {}
  PrinterBase(const PrinterBase &) = delete;
  PrinterBase(PrinterBase &&) = delete;
};

template <typename T, typename = void>
struct is_container : public std::false_type {};
template <typename T>
struct is_container<T, std::void_t<typename T::value_type, typename T::iterator,
                                   typename T::const_iterator,
                                   decltype(std::declval<T>().begin()),
                                   decltype(std::declval<T>().end())>>
    : public std::true_type {};

template <typename T, typename = void>
struct is_default_printable : public std::false_type {};
template <typename T>
struct is_default_printable<
    T, std::void_t<decltype(std::declval<llvm::raw_ostream>()
                            << std::declval<T>())>> : public std::true_type {};
} // namespace detail

template <typename T>
constexpr bool is_container_v = detail::is_container<T>::value;

template <typename T>
constexpr bool is_default_printable_v = detail::is_default_printable<T>::value;

template <typename T, typename Enable = void>
struct Printer : public detail::PrinterBase {
  using detail::PrinterBase::PrinterBase;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Printer &P) {
    return OS << *static_cast<const T *>(P.value);
  }
};

template <typename V>
struct Printer<
    V, std::enable_if_t<std::is_base_of_v<
           llvm::Value, std::remove_pointer_t<typename std::decay_t<V>>>>>
    : public detail::PrinterBase {
  using detail::PrinterBase::PrinterBase;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Printer &P) {
    return OS << llvmIRToString(*static_cast<const V *>(P.value));
  }
};

/*template <typename T>
struct Printer<T, std::enable_if<is_default_printable_v<T>>>
    : public detail::PrinterBase {
  using detail::PrinterBase::PrinterBase;
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Printer &P) {
    return OS << *static_cast<const T *>(P.value);
  }
};*/

template <typename K, typename V>
struct Printer<std::pair<K, V>> : public detail::PrinterBase {
  using detail::PrinterBase::PrinterBase;
  using value_t = std::pair<K, V>;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Printer &P) {
    return OS << "("
              << Printer<K>(&static_cast<const value_t *>(P.value)->first)
              << ", "
              << Printer<V>(&static_cast<const value_t *>(P.value)->second)
              << ")";
  }
};

template <typename... Elems>
struct Printer<std::tuple<Elems...>> : public detail::PrinterBase {
  using detail::PrinterBase::PrinterBase;
  using value_t = std::tuple<Elems...>;

private:
  template <size_t... I>
  static void printTuple(llvm::raw_ostream &OS, const value_t &Tup,
                         std::index_sequence<I...>) {
    OS << "(";
    (..., (OS << (I == 0 ? "" : ", ") << std::get<I>(Tup)));
    OS << ")";
  }

public:
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Printer &P) {
    printTuple(OS, *static_cast<const value_t *>(P.value),
               std::make_index_sequence<sizeof...(Elems)>());
    return OS;
  }
};

template <> struct Printer<llvm::SmallBitVector> : public detail::PrinterBase {
  using detail::PrinterBase::PrinterBase;
  using value_t = llvm::SmallBitVector;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Printer &P) {
    OS << "{";

    auto &bv = *static_cast<const llvm::SmallBitVector *>(P.value);

    auto x = bv.find_first();
    if (x >= 0) {
      OS << Printer<decltype(x)>(&x);

      while ((x = bv.find_next(x)))
        OS << ", " << Printer<decltype(x)>(&x);
    }

    return OS << "}";
  }
};

template <typename T>
struct Printer<T, std::enable_if_t<
                      !std::is_pointer_v<T> /*&& !is_default_printable_v<T>*/ &&
                      is_container_v<T>>> : public detail::PrinterBase {

  using detail::PrinterBase::PrinterBase;
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const Printer &P) {
    OS << "{";
    bool frst = true;
    for (auto &&elem : *static_cast<const T *>(P.value)) {
      if (frst)
        frst = false;
      else
        OS << ", ";

      OS << Printer<std::remove_reference_t<std::remove_cv_t<decltype(elem)>>>(
          &elem);
    }
    return OS << "}";
  }
};

template <typename T>
std::ostream &operator<<(std::ostream &OS, const Printer<T> &P) {
  llvm::raw_os_ostream osos(OS);
  osos << P;
  return OS;
}

template <typename T> Printer(const T *) -> Printer<T>;

} // namespace psr

#endif // PHASAR_UTILS_DEBUGOUTPUT_H_