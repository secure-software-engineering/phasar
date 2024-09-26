/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_EMPTYBASEOPTIMIZATIONUTILS_H_
#define PHASAR_UTILS_EMPTYBASEOPTIMIZATIONUTILS_H_

#include "phasar/Utils/ByRef.h"

#include "llvm/ADT/DenseMapInfo.h"

#include <cstdint>
#include <functional>
#include <type_traits>

namespace psr {
/// A dummy type that takes no space in memory when used with the empty-base
/// optimization or with [[no_unique_address]]
struct EmptyType {
  constexpr friend bool operator==(EmptyType /*LHS*/,
                                   EmptyType /*RHS*/) noexcept {
    return true;
  }
  constexpr friend bool operator!=(EmptyType /*LHS*/,
                                   EmptyType /*RHS*/) noexcept {
    return false;
  }
};

/// A wrapper over a single object that pretends to be a std::pair
template <typename T> struct DummyPair {
  T first; // NOLINT -- Need to have the same interface as std::pair
  [[no_unique_address]] EmptyType second; // NOLINT -- ''

  [[nodiscard]] auto getHashCode() const noexcept {
    return std::hash<T>{}(first);
  }
  template <typename TT = T>
  friend std::enable_if_t<CanEfficientlyPassByValue<TT>, bool>
  operator==(DummyPair LHS,
             DummyPair RHS) noexcept(noexcept(LHS.first == RHS.first)) {
    return LHS.first == RHS.first;
  }

  template <typename TT = T>
  friend std::enable_if_t<!CanEfficientlyPassByValue<TT>, bool>
  operator==(const DummyPair &LHS,
             const DummyPair &RHS) noexcept(noexcept(LHS.first == RHS.first)) {
    return LHS.first == RHS.first;
  }

  template <typename TT = T>
  friend std::enable_if_t<CanEfficientlyPassByValue<TT>, bool>
  operator!=(DummyPair LHS, DummyPair RHS) noexcept(noexcept(LHS == RHS)) {
    return !(LHS == RHS);
  }

  template <typename TT = T>
  friend std::enable_if_t<!CanEfficientlyPassByValue<TT>, bool>
  operator!=(const DummyPair &LHS,
             const DummyPair &RHS) noexcept(noexcept(LHS == RHS)) {
    return !(LHS == RHS);
  }
};
} // namespace psr

namespace llvm {
template <typename T> struct DenseMapInfo<psr::DummyPair<T>> {
  using value_type = psr::DummyPair<T>;

  static value_type getEmptyKey() noexcept {
    return {DenseMapInfo<T>::getEmptyKey(), {}};
  }
  static value_type getTombstoneKey() noexcept {
    return {DenseMapInfo<T>::getTombstoneKey(), {}};
  }
  static auto getHashValue(psr::ByConstRef<value_type> DP) noexcept {
    return DP.getHashCode();
  }
  static bool isEqual(psr::ByConstRef<value_type> LHS,
                      psr::ByConstRef<value_type> RHS) noexcept {
    return DenseMapInfo<T>::isEqual(LHS.first, RHS.first);
  }
};
} // namespace llvm

#endif // PHASAR_UTILS_EMPTYBASEOPTIMIZATIONUTILS_H_
