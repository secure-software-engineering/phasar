/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_DYNAMICALIASSET_H
#define PHASAR_PHASARLLVM_POINTER_DYNAMICALIASSET_H

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"

#include <cstddef>

namespace llvm {
class Value;
} // namespace llvm

namespace psr {

template <typename Container = llvm::DenseSet<const llvm::Value *>>
class DynamicAliasSetConstPtr;
/// A Container** that behaves like a Container*
template <typename Container = llvm::DenseSet<const llvm::Value *>>
class DynamicAliasSetPtr {
public:
  using container_type = Container;

  constexpr DynamicAliasSetPtr() noexcept = default;
  constexpr DynamicAliasSetPtr(std::nullptr_t) noexcept {}
  constexpr DynamicAliasSetPtr(container_type **Value) noexcept : Value(Value) {
    assert(Value != nullptr);
  }
  constexpr DynamicAliasSetPtr(const DynamicAliasSetPtr &) noexcept = default;
  constexpr DynamicAliasSetPtr &
  operator=(const DynamicAliasSetPtr &) noexcept = default;
  ~DynamicAliasSetPtr() noexcept = default;

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
  hash_value(DynamicAliasSetPtr // NOLINT(readability-identifier-naming)
                 Ptr) noexcept {
    return llvm::hash_value(Ptr.Value);
  }

  [[nodiscard]] friend constexpr bool
  operator==(DynamicAliasSetPtr LHS, DynamicAliasSetPtr RHS) noexcept {
    return LHS.Value == RHS.Value;
  }
  [[nodiscard]] friend constexpr bool
  operator!=(DynamicAliasSetPtr LHS, DynamicAliasSetPtr RHS) noexcept {
    return !(LHS == RHS);
  }

  [[nodiscard]] friend constexpr bool
  operator<(DynamicAliasSetPtr LHS, DynamicAliasSetPtr RHS) noexcept {
    return LHS.Value < RHS.Value;
  }
  [[nodiscard]] friend constexpr bool
  operator>(DynamicAliasSetPtr LHS, DynamicAliasSetPtr RHS) noexcept {
    return LHS.Value > RHS.Value;
  }

  [[nodiscard]] friend constexpr bool
  operator<=(DynamicAliasSetPtr LHS, DynamicAliasSetPtr RHS) noexcept {
    return LHS.Value <= RHS.Value;
  }
  [[nodiscard]] friend constexpr bool
  operator>=(DynamicAliasSetPtr LHS, DynamicAliasSetPtr RHS) noexcept {
    return LHS.Value >= RHS.Value;
  }

private:
  friend class DynamicAliasSetConstPtr<container_type>;

  container_type **Value = nullptr;
};

/// A Container const** that behaves like a const Container*.
/// NOTE: This is different to const DynamicAliasSetPtr<Container> which
/// propagates the const through all known layers of indirection
template <typename Container>
class DynamicAliasSetConstPtr : private DynamicAliasSetPtr<Container> {
public:
  using DynamicAliasSetPtr<Container>::DynamicAliasSetPtr;
  using DynamicAliasSetPtr<Container>::operator bool;
  using typename DynamicAliasSetPtr<Container>::container_type;

  constexpr DynamicAliasSetConstPtr(
      DynamicAliasSetPtr<Container> Other) noexcept
      : DynamicAliasSetPtr<Container>(Other) {}

  [[nodiscard]] constexpr const container_type *get() const noexcept {
    return DynamicAliasSetPtr<Container>::get();
  }

  constexpr const container_type *operator->() const noexcept { return get(); }

  constexpr const container_type &operator*() const noexcept {
    return *DynamicAliasSetPtr<Container>::get();
  }

  [[nodiscard]] constexpr container_type const *const *value() const noexcept {
    return DynamicAliasSetPtr<Container>::value();
  }

  [[nodiscard]] friend constexpr llvm::hash_code
  hash_value(DynamicAliasSetConstPtr // NOLINT(readability-identifier-naming)
                 Ptr) noexcept {
    return llvm::hash_value(Ptr.value());
  }

  [[nodiscard]] friend constexpr bool
  operator==(DynamicAliasSetConstPtr LHS,
             DynamicAliasSetConstPtr RHS) noexcept {
    return LHS.value() == RHS.value();
  }
  [[nodiscard]] friend constexpr bool
  operator!=(DynamicAliasSetConstPtr LHS,
             DynamicAliasSetConstPtr RHS) noexcept {
    return !(LHS == RHS);
  }

  [[nodiscard]] friend constexpr bool
  operator<(DynamicAliasSetConstPtr LHS, DynamicAliasSetConstPtr RHS) noexcept {
    return LHS.value() < RHS.value();
  }
  [[nodiscard]] friend constexpr bool
  operator>(DynamicAliasSetConstPtr LHS, DynamicAliasSetConstPtr RHS) noexcept {
    return LHS.value() > RHS.value();
  }

  [[nodiscard]] friend constexpr bool
  operator<=(DynamicAliasSetConstPtr LHS,
             DynamicAliasSetConstPtr RHS) noexcept {
    return LHS.value() <= RHS.value();
  }
  [[nodiscard]] friend constexpr bool
  operator>=(DynamicAliasSetConstPtr LHS,
             DynamicAliasSetConstPtr RHS) noexcept {
    return LHS.value() >= RHS.value();
  }
};

extern template class DynamicAliasSetPtr<>;
extern template class DynamicAliasSetConstPtr<>;
} // namespace psr

namespace std {
template <typename C> struct hash<psr::DynamicAliasSetPtr<C>> {
  constexpr size_t operator()(psr::DynamicAliasSetPtr<C> Ptr) const noexcept {
    return std::hash<C **>{}(Ptr.value());
  }
};
template <typename C> struct hash<psr::DynamicAliasSetConstPtr<C>> {
  constexpr size_t
  operator()(psr::DynamicAliasSetConstPtr<C> Ptr) const noexcept {
    return std::hash<C const *const *>{}(Ptr.value());
  }
};
} // namespace std

namespace llvm {
template <typename C> struct DenseMapInfo<psr::DynamicAliasSetPtr<C>> {
  inline static psr::DynamicAliasSetPtr<C> getEmptyKey() noexcept {
    return DenseMapInfo<C **>::getEmptyKey();
  }
  inline static psr::DynamicAliasSetPtr<C> getTombstoneKey() noexcept {
    return DenseMapInfo<C **>::getTombstoneKey();
  }

  inline static bool isEqual(psr::DynamicAliasSetPtr<C> LHS,
                             psr::DynamicAliasSetPtr<C> RHS) noexcept {
    return LHS == RHS;
  }

  inline static unsigned getHashValue(psr::DynamicAliasSetPtr<C> Ptr) noexcept {
    return hash_value(Ptr);
  }
};
template <typename C> struct DenseMapInfo<psr::DynamicAliasSetConstPtr<C>> {
  inline static psr::DynamicAliasSetConstPtr<C> getEmptyKey() noexcept {
    return DenseMapInfo<C **>::getEmptyKey();
  }
  inline static psr::DynamicAliasSetConstPtr<C> getTombstoneKey() noexcept {
    return DenseMapInfo<C **>::getTombstoneKey();
  }

  inline static bool isEqual(psr::DynamicAliasSetConstPtr<C> LHS,
                             psr::DynamicAliasSetConstPtr<C> RHS) noexcept {
    return LHS == RHS;
  }

  inline static unsigned
  getHashValue(psr::DynamicAliasSetConstPtr<C> Ptr) noexcept {
    return hash_value(Ptr);
  }
};
} // namespace llvm

#endif // PHASAR_PHASARLLVM_POINTER_DYNAMICALIASSET_H
