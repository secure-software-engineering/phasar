/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_PRINTER_H
#define PHASAR_UTILS_PRINTER_H

#include "phasar/Utils/TypeTraits.h"

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <type_traits>

namespace llvm {
class Value;
class Instruction;
class Function;
} // namespace llvm

namespace psr {
namespace detail {
template <typename T>
static constexpr bool IsSomehowPrintable =
    has_str_v<T> || is_llvm_printable_v<T> || has_adl_to_string_v<T>;

template <typename T> decltype(auto) printSomehow(const T &Val) {
  if constexpr (has_str_v<T>) {
    return Val.str();
  } else if constexpr (has_adl_to_string_v<T>) {
    return adl_to_string(Val);
  } else if constexpr (is_llvm_printable_v<T>) {
    std::string Str;
    llvm::raw_string_ostream ROS(Str);
    ROS << Val;
    return Str;
  } else {
    llvm_unreachable(
        "All compilable cases should be handled in the if-chain above");
  }
}
} // namespace detail

/// Stringify the given ICFG node (instruction/statement).
///
/// Default implementation. Provide your own overload to customize this API for
/// your types
template <typename N,
          typename = std::enable_if_t<detail::IsSomehowPrintable<N>>>
[[nodiscard]] decltype(auto) NToString(const N &Node) {
  return detail::printSomehow(Node);
}

/// Stringify the given data-flow fact.
///
/// Default implementation. Provide your own overload to customize this API for
/// your types
template <typename D,
          typename = std::enable_if_t<detail::IsSomehowPrintable<D>>>
[[nodiscard]] decltype(auto) DToString(const D &Fact) {
  return detail::printSomehow(Fact);
}

/// Stringify the given edge value.
///
/// Default implementation. Provide your own overload to customize this API for
/// your types
template <typename L,
          typename = std::enable_if_t<detail::IsSomehowPrintable<L>>>
[[nodiscard]] decltype(auto) LToString(const L &Value) {
  return detail::printSomehow(Value);
}

/// Stringify the given function.
///
/// Default implementation. Provide your own overload to customize this API for
/// your types
template <typename F,
          typename = std::enable_if_t<detail::IsSomehowPrintable<F>>>
[[nodiscard]] std::string FToString(const F &Fun) {
  return detail::printSomehow(Fun);
}

// -- specializations

// --- LLVM
// Note: Provide forward declarations here, such that improper usage will
// definitely lead to an error instead of triggering one of the default
// implementations

/// Stringify the given LLVM Value.
///
/// \remark Link phasar_llvm_utils to use this
[[nodiscard]] std::string NToString(const llvm::Value *V);

/// Stringify the given LLVM Instruction.
///
/// \remark Link phasar_llvm_utils to use this
[[nodiscard]] std::string NToString(const llvm::Instruction *V);

/// Stringify the given LLVM Value.
///
/// \remark Link phasar_llvm_utils to use this
[[nodiscard]] std::string DToString(const llvm::Value *V);

/// Stringify the given LLVM Function.
///
/// \remark Link phasar_llvm_utils to use this
[[nodiscard]] llvm::StringRef FToString(const llvm::Function *V);

} // namespace psr

#endif
