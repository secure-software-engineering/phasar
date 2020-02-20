/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_LATTICEDOMAIN_H_
#define PHASAR_PHASARLLVM_IFDSIDE_LATTICEDOMAIN_H_

#include <iostream>
#include <variant>

namespace psr {

/// Represents the infimum of the lattice:
/// Top is the greatest element that is less than or equal to all elements of
/// the lattice.
struct Top {};

static inline std::ostream &operator<<(std::ostream &OS,
                                       [[maybe_unused]] const Top &T) {
  return OS << "Top";
}

/// Represents the supremum of the lattice:
/// Bottom is the least element that is greater than or equal to all elements
/// of the lattice.
struct Bottom {};

static inline std::ostream &operator<<(std::ostream &OS,
                                       [[maybe_unused]] const Bottom &B) {
  return OS << "Bottom";
}

/// A easy shorthand to construct a complete lattice of L.
template <typename L> using LatticeDomain = std::variant<L, Top, Bottom>;

template <typename L>
inline std::ostream &operator<<(std::ostream &OS, const LatticeDomain<L> &LD) {
  if (auto T = std::get_if<Top>(&LD)) {
    return OS << *T;
  }
  if (auto B = std::get_if<Bottom>(&LD)) {
    return OS << *B;
  }
  return OS << std::get<L>(LD);
}

template <typename L>
inline bool operator==(const LatticeDomain<L> &Lhs,
                       const LatticeDomain<L> &Rhs) {
  if (std::holds_alternative<Top>(Lhs) && std::holds_alternative<Top>(Rhs)) {
    return true;
  }
  if (std::holds_alternative<Bottom>(Lhs) &&
      std::holds_alternative<Bottom>(Rhs)) {
    return true;
  }
  if (auto LhsPtr = std::get_if<L>(&Lhs)) {
    if (auto RhsPtr = std::get_if<L>(&Rhs)) {
      return *LhsPtr == *RhsPtr;
    }
  }
  return false;
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
  if (std::holds_alternative<Top>(Lhs)) {
    return true;
  }
  if (std::holds_alternative<Top>(Rhs)) {
    return false;
  }
  if (auto LhsPtr = std::get_if<L>(&Lhs)) {
    if (auto RhsPtr = std::get_if<L>(&Rhs)) {
      return *LhsPtr < *RhsPtr;
    }
  }
  if (std::holds_alternative<Bottom>(Lhs)) {
    return false;
  }
  if (std::holds_alternative<Bottom>(Rhs)) {
    return true;
  }
  return false;
}

} // namespace psr

#endif
