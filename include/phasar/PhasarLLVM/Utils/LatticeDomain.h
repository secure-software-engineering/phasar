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

#include <type_traits>
#include <variant>

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/Utils/TypeTraits.h"

namespace psr {

/// Represents the infimum of the lattice:
/// Top is the greatest element that is less than or equal to all elements of
/// the lattice.
struct Top {};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Top /*unused*/) {
  return OS << "Top";
}

/// Represents the supremum of the lattice:
/// Bottom is the least element that is greater than or equal to all elements
/// of the lattice.
struct Bottom {};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Bottom /*unused*/) {
  return OS << "Bottom";
}

/// A easy shorthand to construct a complete lattice of L.
template <typename L>
struct LatticeDomain : public std::variant<Top, L, Bottom> {
  using std::variant<Top, L, Bottom>::variant;

  [[nodiscard]] inline bool isBottom() const noexcept {
    return std::holds_alternative<Bottom>(*this);
  }
  [[nodiscard]] inline bool isTop() const noexcept {
    return std::holds_alternative<Top>(*this);
  }
  [[nodiscard]] inline L *getValueOrNull() noexcept {
    return std::get_if<L>(this);
  }
  [[nodiscard]] inline const L *getValueOrNull() const noexcept {
    return std::get_if<L>(this);
  }
};

template <typename L,
          typename = std::void_t<decltype(std::declval<llvm::raw_ostream &>()
                                          << std::declval<L>())>>
inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                     const LatticeDomain<L> &LD) {
  if (LD.isBottom()) {
    return OS << "Bottom";
  }
  if (LD.isTop()) {
    return OS << "Top";
  }

  const auto *Val = LD.getValueOrNull();
  assert(Val && "Only alternative remaining is L");
  return OS << *Val;
}

template <typename L>
inline bool operator==(const LatticeDomain<L> &Lhs,
                       const LatticeDomain<L> &Rhs) {
  if (Lhs.index() != Rhs.index()) {
    return false;
  }
  if (auto LhsPtr = Lhs.getValueOrNull()) {
    /// No need to check whether Lhs is an L; the indices are already the same
    return *LhsPtr == *Rhs.getValueOrNull();
  }
  return true;
}

template <
    typename L, typename LL,
    typename = std::void_t<decltype(std::declval<LL>() == std::declval<L>())>>
inline bool operator==(const LL &Lhs, const LatticeDomain<L> Rhs) {
  if (auto RVal = Rhs.getValueOrNull()) {
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
  /// Top < (Lhs::L < Rhs::L) < Bottom
  if (Rhs.isTop()) {
    return false;
  }
  if (Lhs.isTop()) {
    return true;
  }
  if (auto LhsPtr = Lhs.getValueOrNull()) {
    if (auto RhsPtr = Rhs.getValueOrNull()) {
      return *LhsPtr < *RhsPtr;
    }
  }
  if (Lhs.isBottom()) {
    return false;
  }
  if (Rhs.isBottom()) {
    return true;
  }
  llvm_unreachable("All comparision cases should be handled above.");
}

} // namespace psr

#endif
