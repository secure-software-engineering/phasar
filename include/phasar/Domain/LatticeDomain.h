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
#include "phasar/Utils/DebugOutput.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <functional>
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

  [[nodiscard]] constexpr bool isBottom() const noexcept {
    return std::holds_alternative<Bottom>(*this);
  }
  [[nodiscard]] constexpr bool isTop() const noexcept {
    return std::holds_alternative<Top>(*this);
  }

  [[nodiscard]] constexpr L *getValueOrNull() noexcept {
    return std::get_if<L>(this);
  }
  [[nodiscard]] constexpr const L *getValueOrNull() const noexcept {
    return std::get_if<L>(this);
  }

  friend llvm::hash_code hash_value(const LatticeDomain &LD) noexcept
    requires is_llvm_hashable_v<L>
  { // NOLINT
    if (LD.isBottom()) {
      return llvm::hash_value(INTPTR_MAX);
    }
    if (LD.isTop()) {
      return llvm::hash_value(INTPTR_MIN);
    }
    return hash_value(std::get<L>(LD));
  }

  [[nodiscard]] constexpr L &assertGetValue() noexcept {
    assert(std::holds_alternative<L>(*this));
    return std::get<L>(*this);
  }
  [[nodiscard]] constexpr const L &assertGetValue() const noexcept {
    assert(std::holds_alternative<L>(*this));
    return std::get<L>(*this);
  }

  template <typename TransformFn, typename... ArgsT>
  constexpr void onValue(TransformFn Transform, ArgsT &&...Args) {
    if (auto *Val = getValueOrNull()) {
      std::invoke(std::move(Transform), *Val, PSR_FWD(Args)...);
    }
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const LatticeDomain &LD) {
    if (LD.isBottom()) {
      return OS << "Bottom";
    }
    if (LD.isTop()) {
      return OS << "Top";
    }

    const auto *Val = LD.getValueOrNull();
    assert(Val && "Only alternative remaining is L");
    if constexpr (is_llvm_printable_v<L>) {
      return OS << *Val;
    } else {
      return OS << PrettyPrinter{*Val};
    }
  }

  friend std::ostream &operator<<(std::ostream &OS, const LatticeDomain &LD) {
    llvm::raw_os_ostream ROS(OS);
    ROS << LD;
    return OS;
  }

  constexpr bool operator==(const LatticeDomain &Rhs) const {
    if (this->index() != Rhs.index()) {
      return false;
    }
    if (auto LhsPtr = this->getValueOrNull()) {
      /// No need to check whether Rhs is an L; the indices are already the same
      return *LhsPtr == *Rhs.getValueOrNull();
    }
    return true;
  }

  template <typename LL>
    requires AreEqualityComparable<L, LL>
  constexpr bool operator==(const LL &Rhs) const {
    if (auto LVal = this->getValueOrNull()) {
      return *LVal == Rhs;
    }
    return false;
  }

  constexpr bool operator==(Bottom /*Rhs*/) const noexcept {
    return this->isBottom();
  }

  constexpr bool operator==(Top /*Rhs*/) const noexcept {
    return this->isTop();
  }

  constexpr bool operator<(const LatticeDomain &Rhs) const {
    /// Top < (Lhs::L < Rhs::L) < Bottom
    if (Rhs.isTop()) {
      return false;
    }
    if (this->isTop()) {
      return true;
    }
    if (auto LhsPtr = this->getValueOrNull()) {
      if (auto RhsPtr = Rhs.getValueOrNull()) {
        return *LhsPtr < *RhsPtr;
      }
    } else if (this->isBottom()) {
      return false;
    }
    if (Rhs.isBottom()) {
      return true;
    }
    llvm_unreachable("All comparison cases should be handled above.");
  }
};

template <typename L> struct JoinLatticeTraits<LatticeDomain<L>> {
  using l_t = L;
  static constexpr Bottom bottom() noexcept { return {}; }
  static constexpr Top top() noexcept { return {}; }
  static constexpr LatticeDomain<L> join(ByConstRef<LatticeDomain<l_t>> LHS,
                                         ByConstRef<LatticeDomain<l_t>> RHS) {
    // Top < (Lhs::l_t < Rhs::l_t) < Bottom
    if (LHS.isTop() || LHS == RHS) {
      return RHS;
    }

    if (RHS.isTop()) {
      return LHS;
    }

    if constexpr (has_adl_join<l_t>) {
      if (auto LhsPtr = LHS.getValueOrNull()) {
        if (auto RhsPtr = RHS.getValueOrNull()) {
          return psr::adl_join(*LhsPtr, *RhsPtr);
        }
      }
    }

    return Bottom{};
  }
};

/// If we know that a stored L value is never Top or Bottom, we don't need to
/// store the discriminator of the std::variant.
template <typename L>
  requires(std::is_nothrow_constructible_v<LatticeDomain<L>, const L &> ||
           !std::is_nothrow_copy_constructible_v<LatticeDomain<L>>)
struct NonTopBotValue<LatticeDomain<L>> {
  using type = L;

  constexpr static L unwrap(LatticeDomain<L> Value) noexcept(
      std::is_nothrow_move_constructible_v<L>) {
    return std::get<L>(std::move(Value));
  }
};

} // namespace psr

namespace std {
template <typename L> struct hash<psr::LatticeDomain<L>> {
  constexpr size_t operator()(const psr::LatticeDomain<L> &LD) noexcept {
    if (LD.isBottom()) {
      return SIZE_MAX;
    }
    if (LD.isTop()) {
      return SIZE_MAX - 1;
    }
    assert(LD.getValueOrNull() != nullptr);
    if constexpr (psr::is_std_hashable_v<L>) {
      return std::hash<L>{}(*LD.getValueOrNull());
    } else {
      using llvm::hash_value;
      return hash_value(*LD.getValueOrNull());
    }
  }
};
} // namespace std

#endif
