/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_DYNAMICPOINTSTOSET_H
#define PHASAR_PHASARLLVM_POINTER_DYNAMICPOINTSTOSET_H

#include <cstddef>

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"

namespace llvm {
class Value;
} // namespace llvm

namespace psr {

template <typename Container = llvm::DenseSet<const llvm::Value *>>
class DynamicPointsToSetConstPtr;
/// A Container** that behaves like a Container*
template <typename Container = llvm::DenseSet<const llvm::Value *>>
class DynamicPointsToSetPtr {
public:
  using container_type = Container;

  constexpr DynamicPointsToSetPtr() noexcept = default;
  constexpr DynamicPointsToSetPtr(std::nullptr_t) noexcept {}
  constexpr DynamicPointsToSetPtr(container_type **Value) noexcept
      : Value(Value) {
    assert(Value != nullptr);
  }
  constexpr DynamicPointsToSetPtr(const DynamicPointsToSetPtr &) noexcept =
      default;
  constexpr DynamicPointsToSetPtr &
  operator=(const DynamicPointsToSetPtr &) noexcept = default;
  ~DynamicPointsToSetPtr() noexcept = default;

  [[nodiscard]] constexpr container_type *get() noexcept { return *Value; }
  [[nodiscard]] constexpr const container_type *get() const noexcept {
    return *Value;
  }

  constexpr container_type *operator->() noexcept { return get(); }
  constexpr const container_type *operator->() const noexcept { return get(); }

  constexpr container_type &operator*() noexcept { return **Value; }
  constexpr const container_type &operator*() const noexcept { return **Value; }

  constexpr operator bool() const noexcept {
    return Value && *Value != nullptr;
  }

  [[nodiscard]] constexpr container_type **value() noexcept { return Value; }
  [[nodiscard]] constexpr container_type const *const *value() const noexcept {
    return Value;
  }

  [[nodiscard]] friend constexpr llvm::hash_code
  hash_value(DynamicPointsToSetPtr // NOLINT(readability-identifier-naming)
                 Ptr) noexcept {
    return llvm::hash_value(Ptr.Value);
  }

  [[nodiscard]] friend constexpr bool
  operator==(DynamicPointsToSetPtr LHS, DynamicPointsToSetPtr RHS) noexcept {
    return LHS.Value == RHS.Value;
  }
  [[nodiscard]] friend constexpr bool
  operator!=(DynamicPointsToSetPtr LHS, DynamicPointsToSetPtr RHS) noexcept {
    return !(LHS == RHS);
  }

  [[nodiscard]] friend constexpr bool
  operator<(DynamicPointsToSetPtr LHS, DynamicPointsToSetPtr RHS) noexcept {
    return LHS.Value < RHS.Value;
  }
  [[nodiscard]] friend constexpr bool
  operator>(DynamicPointsToSetPtr LHS, DynamicPointsToSetPtr RHS) noexcept {
    return LHS.Value > RHS.Value;
  }

  [[nodiscard]] friend constexpr bool
  operator<=(DynamicPointsToSetPtr LHS, DynamicPointsToSetPtr RHS) noexcept {
    return LHS.Value <= RHS.Value;
  }
  [[nodiscard]] friend constexpr bool
  operator>=(DynamicPointsToSetPtr LHS, DynamicPointsToSetPtr RHS) noexcept {
    return LHS.Value >= RHS.Value;
  }

private:
  friend class DynamicPointsToSetConstPtr<container_type>;

  container_type **Value = nullptr;
};

/// A Container const** that behaves like a const Container*.
/// NOTE: This is different to const DynamicPointsToSetPtr<Container> which
/// propagates the const through all known layers of indirection
template <typename Container>
class DynamicPointsToSetConstPtr : private DynamicPointsToSetPtr<Container> {
public:
  using DynamicPointsToSetPtr<Container>::DynamicPointsToSetPtr;
  using DynamicPointsToSetPtr<Container>::operator bool;
  using typename DynamicPointsToSetPtr<Container>::container_type;

  constexpr DynamicPointsToSetConstPtr(
      DynamicPointsToSetPtr<Container> Other) noexcept
      : DynamicPointsToSetPtr<Container>(Other) {}

  [[nodiscard]] constexpr const container_type *get() const noexcept {
    return DynamicPointsToSetPtr<Container>::get();
  }

  constexpr const container_type *operator->() const noexcept { return get(); }

  constexpr const container_type &operator*() const noexcept {
    return *DynamicPointsToSetPtr<Container>::get();
  }

  [[nodiscard]] constexpr container_type const *const *value() const noexcept {
    return DynamicPointsToSetPtr<Container>::value();
  }

  [[nodiscard]] friend constexpr llvm::hash_code
  hash_value(DynamicPointsToSetConstPtr // NOLINT(readability-identifier-naming)
                 Ptr) noexcept {
    return llvm::hash_value(Ptr.value());
  }

  [[nodiscard]] friend constexpr bool
  operator==(DynamicPointsToSetConstPtr LHS,
             DynamicPointsToSetConstPtr RHS) noexcept {
    return LHS.value() == RHS.value();
  }
  [[nodiscard]] friend constexpr bool
  operator!=(DynamicPointsToSetConstPtr LHS,
             DynamicPointsToSetConstPtr RHS) noexcept {
    return !(LHS == RHS);
  }

  [[nodiscard]] friend constexpr bool
  operator<(DynamicPointsToSetConstPtr LHS,
            DynamicPointsToSetConstPtr RHS) noexcept {
    return LHS.value() < RHS.value();
  }
  [[nodiscard]] friend constexpr bool
  operator>(DynamicPointsToSetConstPtr LHS,
            DynamicPointsToSetConstPtr RHS) noexcept {
    return LHS.value() > RHS.value();
  }

  [[nodiscard]] friend constexpr bool
  operator<=(DynamicPointsToSetConstPtr LHS,
             DynamicPointsToSetConstPtr RHS) noexcept {
    return LHS.value() <= RHS.value();
  }
  [[nodiscard]] friend constexpr bool
  operator>=(DynamicPointsToSetConstPtr LHS,
             DynamicPointsToSetConstPtr RHS) noexcept {
    return LHS.value() >= RHS.value();
  }
};

extern template class DynamicPointsToSetPtr<>;
extern template class DynamicPointsToSetConstPtr<>;
} // namespace psr

namespace std {
template <typename C> struct hash<psr::DynamicPointsToSetPtr<C>> {
  constexpr size_t
  operator()(psr::DynamicPointsToSetPtr<C> Ptr) const noexcept {
    return std::hash<C **>{}(Ptr.value());
  }
};
template <typename C> struct hash<psr::DynamicPointsToSetConstPtr<C>> {
  constexpr size_t
  operator()(psr::DynamicPointsToSetConstPtr<C> Ptr) const noexcept {
    return std::hash<C const *const *>{}(Ptr.value());
  }
};
} // namespace std

namespace llvm {
template <typename C> struct DenseMapInfo<psr::DynamicPointsToSetPtr<C>> {
  inline static psr::DynamicPointsToSetPtr<C> getEmptyKey() noexcept {
    return DenseMapInfo<C **>::getEmptyKey();
  }
  inline static psr::DynamicPointsToSetPtr<C> getTombstoneKey() noexcept {
    return DenseMapInfo<C **>::getTombstoneKey();
  }

  inline static bool isEqual(psr::DynamicPointsToSetPtr<C> LHS,
                             psr::DynamicPointsToSetPtr<C> RHS) noexcept {
    return LHS == RHS;
  }

  inline static unsigned
  getHashValue(psr::DynamicPointsToSetPtr<C> Ptr) noexcept {
    return hash_value(Ptr);
  }
};
template <typename C> struct DenseMapInfo<psr::DynamicPointsToSetConstPtr<C>> {
  inline static psr::DynamicPointsToSetConstPtr<C> getEmptyKey() noexcept {
    return DenseMapInfo<C **>::getEmptyKey();
  }
  inline static psr::DynamicPointsToSetConstPtr<C> getTombstoneKey() noexcept {
    return DenseMapInfo<C **>::getTombstoneKey();
  }

  inline static bool isEqual(psr::DynamicPointsToSetConstPtr<C> LHS,
                             psr::DynamicPointsToSetConstPtr<C> RHS) noexcept {
    return LHS == RHS;
  }

  inline static unsigned
  getHashValue(psr::DynamicPointsToSetConstPtr<C> Ptr) noexcept {
    return hash_value(Ptr);
  }
};
} // namespace llvm

#endif // PHASAR_PHASARLLVM_POINTER_DYNAMICPOINTSTOSET_H
