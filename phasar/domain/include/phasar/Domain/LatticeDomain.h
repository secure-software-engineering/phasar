/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_DOMAIN_LATTICEDOMAIN_H
#define PHASAR_DOMAIN_LATTICEDOMAIN_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <ostream>
#include <variant>

namespace psr {

/// Represents the infimum of the lattice:
/// Top is the greatest element that is less than or equal to all elements of
/// the lattice.
struct Top {
  friend constexpr bool operator==(Top /*unused*/, Top /*unused*/) noexcept {
    return true;
  }
  friend constexpr bool operator!=(Top /*unused*/, Top /*unused*/) noexcept {
    return false;
  }
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Top /*unused*/) {
    return OS << "Top";
  }
};

/// Represents the supremum of the lattice:
/// Bottom is the least element that is greater than or equal to all elements
/// of the lattice.
struct Bottom {
  friend constexpr bool operator==(Bottom /*unused*/,
                                   Bottom /*unused*/) noexcept {
    return true;
  }
  friend constexpr bool operator!=(Bottom /*unused*/,
                                   Bottom /*unused*/) noexcept {
    return false;
  }
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       Bottom /*unused*/) {
    return OS << "Bottom";
  }
};

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
  template <typename LL = L,
            typename = std::enable_if_t<is_llvm_hashable_v<LL>>>
  friend llvm::hash_code
  hash_value(const LatticeDomain &LD) noexcept { // NOLINT
    if (LD.isBottom()) {
      return llvm::hash_value(INTPTR_MAX);
    }
    if (LD.isTop()) {
      return llvm::hash_value(INTPTR_MIN);
    }
    return hash_value(std::get<L>(LD));
  }

  [[nodiscard]] inline L &assertGetValue() noexcept {
    assert(std::holds_alternative<L>(*this));
    return std::get<L>(*this);
  }
  [[nodiscard]] inline const L &assertGetValue() const noexcept {
    assert(std::holds_alternative<L>(*this));
    return std::get<L>(*this);
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
inline std::ostream &operator<<(std::ostream &OS, const LatticeDomain<L> &LD) {
  llvm::raw_os_ostream ROS(OS);
  ROS << LD;
  return OS;
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
inline bool operator==(const LL &Lhs, const LatticeDomain<L> &Rhs) {
  if (auto RVal = Rhs.getValueOrNull()) {
    return Lhs == *RVal;
  }
  return false;
}

template <
    typename L, typename LL,
    typename = std::void_t<decltype(std::declval<LL>() == std::declval<L>())>>
inline bool operator==(const LatticeDomain<L> &Lhs, const LL &Rhs) {
  return Rhs == Lhs;
}

template <typename L>
inline bool operator==(const LatticeDomain<L> &Lhs, Bottom /*Rhs*/) noexcept {
  return Lhs.isBottom();
}

template <typename L>
inline bool operator==(const LatticeDomain<L> &Lhs, Top /*Rhs*/) noexcept {
  return Lhs.isTop();
}

template <typename L>
inline bool operator==(Bottom /*Lhs*/, const LatticeDomain<L> &Rhs) noexcept {
  return Rhs.isBottom();
}

template <typename L>
inline bool operator==(Top /*Lhs*/, const LatticeDomain<L> &Rhs) noexcept {
  return Rhs.isTop();
}

#if __cplusplus < 202002L

// With C++20 inequality is defaulted if equality is provided

template <typename L>
inline bool operator!=(const LatticeDomain<L> &Lhs,
                       const LatticeDomain<L> &Rhs) {
  return !(Lhs == Rhs);
}

template <
    typename L, typename LL,
    typename = std::void_t<decltype(std::declval<LL>() == std::declval<L>())>>
inline bool operator!=(const LL &Lhs, const LatticeDomain<L> Rhs) {
  return !(Lhs == Rhs);
}

template <
    typename L, typename LL,
    typename = std::void_t<decltype(std::declval<LL>() == std::declval<L>())>>
inline bool operator!=(const LatticeDomain<L> Lhs, const LL &Rhs) {
  return !(Rhs == Lhs);
}

template <typename L>
inline bool operator!=(const LatticeDomain<L> &Lhs, Bottom /*Rhs*/) noexcept {
  return !(Lhs == Bottom{});
}

template <typename L>
inline bool operator!=(const LatticeDomain<L> &Lhs, Top /*Rhs*/) noexcept {
  return !(Lhs == Top{});
}

template <typename L>
inline bool operator!=(Bottom /*Lhs*/, const LatticeDomain<L> &Rhs) noexcept {
  return !(Bottom{} == Rhs);
}

template <typename L>
inline bool operator!=(Top /*Lhs*/, const LatticeDomain<L> &Rhs) noexcept {
  return !(Top{} == Rhs);
}

#endif

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
  } else if (Lhs.isBottom()) {
    return false;
  }
  if (Rhs.isBottom()) {
    return true;
  }
  llvm_unreachable("All comparison cases should be handled above.");
}

template <typename L> struct JoinLatticeTraits<LatticeDomain<L>> {
  using l_t = L;
  static constexpr Bottom bottom() noexcept { return {}; }
  static constexpr Top top() noexcept { return {}; }
  static LatticeDomain<L> join(ByConstRef<LatticeDomain<l_t>> LHS,
                               ByConstRef<LatticeDomain<l_t>> RHS) {
    // Top < (Lhs::l_t < Rhs::l_t) < Bottom
    if (LHS.isTop() || LHS == RHS) {
      return RHS;
    }

    if (RHS.isTop()) {
      return LHS;
    }

    return Bottom{};
  }
};

/// If we know that a stored L value is never Top or Bottom, we don't need to
/// store the discriminator of the std::variant.
template <typename L>
struct NonTopBotValue<
    LatticeDomain<L>,
    std::enable_if_t<
        std::is_nothrow_constructible_v<LatticeDomain<L>, const L &> ||
        !std::is_nothrow_copy_constructible_v<LatticeDomain<L>>>> {
  using type = L;

  static L unwrap(LatticeDomain<L> Value) noexcept(
      std::is_nothrow_move_constructible_v<L>) {
    return std::get<L>(std::move(Value));
  }
};

} // namespace psr

#endif
