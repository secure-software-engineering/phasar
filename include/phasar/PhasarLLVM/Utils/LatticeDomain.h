/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_UTILS_LATTICEDOMAIN_H
#define PHASAR_PHASARLLVM_UTILS_LATTICEDOMAIN_H

#include "llvm/Support/ErrorHandling.h"

#include <iostream>
#include <type_traits>
#include <variant>

namespace psr {

/// Represents the infimum of the lattice:
/// Top is the greatest element that is less than or equal to all elements of
/// the lattice.
struct Top {};

static inline std::ostream &operator<<(std::ostream &OS, Top /*unused*/) {
  return OS << "Top";
}

/// Represents the supremum of the lattice:
/// Bottom is the least element that is greater than or equal to all elements
/// of the lattice.
struct Bottom {};

static inline std::ostream &operator<<(std::ostream &OS, Bottom /*unused*/) {
  return OS << "Bottom";
}

/// A easy shorthand to construct a complete lattice of L.
template <typename L> using LatticeDomain = std::variant<L, Top, Bottom>;

template <typename L>
inline std::ostream &operator<<(std::ostream &OS, const LatticeDomain<L> &LD) {
  std::visit([&OS](const auto &LVal) { OS << LVal; }, LD);
  return OS;
}

template <typename L>
inline bool operator==(const LatticeDomain<L> &Lhs,
                       const LatticeDomain<L> &Rhs) {
  if (Lhs.index() != Rhs.index()) {
    return false;
  }

  if (auto LhsPtr = std::get_if<L>(&Lhs)) {
    if (auto RhsPtr = std::get_if<L>(&Rhs)) {
      return *LhsPtr == *RhsPtr;
    }
  }

  return true;
}

template <
    typename L, typename LL,
    typename = std::void_t<decltype(std::declval<LL>() == std::declval<L>())>>
inline bool operator==(const LL &Lhs, const LatticeDomain<L> Rhs) {
  if (const auto *RVal = std::get_if<L>(&Rhs)) {
    return Lhs == *RVal;
  }
  return false;
}

template <
    typename L, typename LL,
    typename = std::void_t<decltype(std::declval<LL>() == std::declval<L>())>>
inline bool operator==(const LatticeDomain<L> Lhs, const LL &Rhs) {
  return Rhs == Lhs;
}

template <typename L>
inline bool operator!=(const LatticeDomain<L> &Lhs,
                       const LatticeDomain<L> &Rhs) {
  return !(Lhs == Rhs);
}

template <typename L>
inline bool operator<(const LatticeDomain<L> &Lhs,
                      const LatticeDomain<L> &Rhs) {
  // Top < (Lhs::L < Rhs::L) < Bottom
  if (std::holds_alternative<Top>(Rhs)) {
    return false;
  }
  if (std::holds_alternative<Top>(Lhs)) {
    return true;
  }

  if (auto LhsPtr = std::get_if<L>(&Lhs)) {
    if (auto RhsPtr = std::get_if<L>(&Rhs)) {
      return *LhsPtr < *RhsPtr;
    }
  }

  if (std::holds_alternative<Bottom>(Rhs)) {
    return !std::holds_alternative<Bottom>(Lhs);
  }
  if (std::holds_alternative<Bottom>(Lhs)) {
    return false;
  }
  llvm_unreachable("All comparision cases should be handled above.");
}

} // namespace psr

#endif
