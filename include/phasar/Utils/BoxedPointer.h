/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_BOXEDPOINTER_H
#define PHASAR_UTILS_BOXEDPOINTER_H

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"

#include <cstddef>

namespace llvm {
class Value;
} // namespace llvm

namespace psr {

template <typename T> class BoxedConstPtr;
/// A T** that behaves like a T*. Used as AliasSetPtr in LLVMAliasSet
template <typename T> class BoxedPtr {
public:
  using value_type = T;

  constexpr BoxedPtr() noexcept = default;
  constexpr BoxedPtr(std::nullptr_t) noexcept {}
  constexpr BoxedPtr(value_type **Value) noexcept : Value(Value) {
    assert(Value != nullptr);
  }
  constexpr BoxedPtr(const BoxedPtr &) noexcept = default;
  constexpr BoxedPtr &operator=(const BoxedPtr &) noexcept = default;
  ~BoxedPtr() noexcept = default;

  [[nodiscard]] constexpr value_type *get() noexcept { return *Value; }
  [[nodiscard]] constexpr const value_type *get() const noexcept {
    return *Value;
  }

  constexpr value_type *operator->() noexcept { return get(); }
  constexpr const value_type *operator->() const noexcept { return get(); }

  constexpr value_type &operator*() noexcept { return **Value; }
  constexpr const value_type &operator*() const noexcept { return **Value; }

  constexpr operator bool() const noexcept {
    return Value && *Value != nullptr;
  }

  [[nodiscard]] constexpr value_type **value() noexcept { return Value; }
  [[nodiscard]] constexpr value_type const *const *value() const noexcept {
    return Value;
  }

  [[nodiscard]] friend constexpr llvm::hash_code
  hash_value(BoxedPtr // NOLINT(readability-identifier-naming)
                 Ptr) noexcept {
    return llvm::hash_value(Ptr.Value);
  }

  [[nodiscard]] friend constexpr bool operator==(BoxedPtr LHS,
                                                 BoxedPtr RHS) noexcept {
    return LHS.Value == RHS.Value;
  }
  [[nodiscard]] friend constexpr bool operator!=(BoxedPtr LHS,
                                                 BoxedPtr RHS) noexcept {
    return !(LHS == RHS);
  }

  [[nodiscard]] friend constexpr bool operator<(BoxedPtr LHS,
                                                BoxedPtr RHS) noexcept {
    return LHS.Value < RHS.Value;
  }
  [[nodiscard]] friend constexpr bool operator>(BoxedPtr LHS,
                                                BoxedPtr RHS) noexcept {
    return LHS.Value > RHS.Value;
  }

  [[nodiscard]] friend constexpr bool operator<=(BoxedPtr LHS,
                                                 BoxedPtr RHS) noexcept {
    return LHS.Value <= RHS.Value;
  }
  [[nodiscard]] friend constexpr bool operator>=(BoxedPtr LHS,
                                                 BoxedPtr RHS) noexcept {
    return LHS.Value >= RHS.Value;
  }

private:
  friend class BoxedConstPtr<value_type>;

  value_type **Value = nullptr;
};

/// A T const** that behaves like a const T*.
/// NOTE: This is different to const BoxedPtr<T> which
/// propagates the const through all known layers of indirection
template <typename T> class BoxedConstPtr : private BoxedPtr<T> {
public:
  using BoxedPtr<T>::BoxedPtr;
  using BoxedPtr<T>::operator bool;
  using typename BoxedPtr<T>::value_type;

  constexpr BoxedConstPtr(BoxedPtr<T> Other) noexcept : BoxedPtr<T>(Other) {}

  [[nodiscard]] constexpr const value_type *get() const noexcept {
    return BoxedPtr<T>::get();
  }

  constexpr const value_type *operator->() const noexcept { return get(); }

  constexpr const value_type &operator*() const noexcept {
    return *BoxedPtr<T>::get();
  }

  [[nodiscard]] constexpr value_type const *const *value() const noexcept {
    return BoxedPtr<T>::value();
  }

  [[nodiscard]] friend constexpr llvm::hash_code
  hash_value(BoxedConstPtr // NOLINT(readability-identifier-naming)
                 Ptr) noexcept {
    return llvm::hash_value(Ptr.value());
  }

  [[nodiscard]] friend constexpr bool operator==(BoxedConstPtr LHS,
                                                 BoxedConstPtr RHS) noexcept {
    return LHS.value() == RHS.value();
  }
  [[nodiscard]] friend constexpr bool operator!=(BoxedConstPtr LHS,
                                                 BoxedConstPtr RHS) noexcept {
    return !(LHS == RHS);
  }

  [[nodiscard]] friend constexpr bool operator<(BoxedConstPtr LHS,
                                                BoxedConstPtr RHS) noexcept {
    return LHS.value() < RHS.value();
  }
  [[nodiscard]] friend constexpr bool operator>(BoxedConstPtr LHS,
                                                BoxedConstPtr RHS) noexcept {
    return LHS.value() > RHS.value();
  }

  [[nodiscard]] friend constexpr bool operator<=(BoxedConstPtr LHS,
                                                 BoxedConstPtr RHS) noexcept {
    return LHS.value() <= RHS.value();
  }
  [[nodiscard]] friend constexpr bool operator>=(BoxedConstPtr LHS,
                                                 BoxedConstPtr RHS) noexcept {
    return LHS.value() >= RHS.value();
  }
};

} // namespace psr

namespace std {
template <typename C> struct hash<psr::BoxedPtr<C>> {
  constexpr size_t operator()(psr::BoxedPtr<C> Ptr) const noexcept {
    return std::hash<C **>{}(Ptr.value());
  }
};
template <typename C> struct hash<psr::BoxedConstPtr<C>> {
  constexpr size_t operator()(psr::BoxedConstPtr<C> Ptr) const noexcept {
    return std::hash<C const *const *>{}(Ptr.value());
  }
};
} // namespace std

namespace llvm {
template <typename C> struct DenseMapInfo<psr::BoxedPtr<C>> {
  inline static psr::BoxedPtr<C> getEmptyKey() noexcept {
    return DenseMapInfo<C **>::getEmptyKey();
  }
  inline static psr::BoxedPtr<C> getTombstoneKey() noexcept {
    return DenseMapInfo<C **>::getTombstoneKey();
  }

  inline static bool isEqual(psr::BoxedPtr<C> LHS,
                             psr::BoxedPtr<C> RHS) noexcept {
    return LHS == RHS;
  }

  inline static unsigned getHashValue(psr::BoxedPtr<C> Ptr) noexcept {
    return hash_value(Ptr);
  }
};
template <typename C> struct DenseMapInfo<psr::BoxedConstPtr<C>> {
  inline static psr::BoxedConstPtr<C> getEmptyKey() noexcept {
    return DenseMapInfo<C **>::getEmptyKey();
  }
  inline static psr::BoxedConstPtr<C> getTombstoneKey() noexcept {
    return DenseMapInfo<C **>::getTombstoneKey();
  }

  inline static bool isEqual(psr::BoxedConstPtr<C> LHS,
                             psr::BoxedConstPtr<C> RHS) noexcept {
    return LHS == RHS;
  }

  inline static unsigned getHashValue(psr::BoxedConstPtr<C> Ptr) noexcept {
    return hash_value(Ptr);
  }
};
} // namespace llvm

#endif // PHASAR_UTILS_BOXEDPOINTER_H
