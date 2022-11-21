/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_MAYBEUNIQUEPTR_H_
#define PHASAR_UTILS_MAYBEUNIQUEPTR_H_

#include "llvm/ADT/PointerIntPair.h"

#include <memory>
#include <type_traits>
#include <utility>

namespace psr {

namespace detail {
template <typename T, bool RequireAlignment> class MaybeUniquePtrBase {
protected:
  struct PointerBoolPairFallback {
    T *Pointer = nullptr;
    bool Flag = false;

    /// Compatibility with llvm::PointerIntPair:
    [[nodiscard]] T *getPointer() const noexcept { return Pointer; }
    [[nodiscard]] bool getInt() const noexcept { return Flag; }
    void setInt(bool Flag) noexcept { this->Flag = Flag; }
  };

  std::conditional_t<(alignof(T) > 1), llvm::PointerIntPair<T *, 1, bool>,
                     PointerBoolPairFallback>
      Data{};

  MaybeUniquePtrBase(T *Ptr, bool Owns) noexcept : Data{Ptr, Owns} {}
  MaybeUniquePtrBase() noexcept = default;
};

template <typename T> class MaybeUniquePtrBase<T, true> {
protected:
  llvm::PointerIntPair<T *, 1, bool> Data{};

  MaybeUniquePtrBase(T *Ptr, bool Owns) noexcept : Data{Ptr, Owns} {}
  MaybeUniquePtrBase() noexcept = default;
};
} // namespace detail

/// A smart-pointer, similar to std::unique_ptr that can be used as both,
/// owning and non-owning pointer.
///
/// \tparam T The pointee type
/// \tparam RequireAlignment If true, the datastructure only works if alignof(T)
/// > 1 holds. Enables incomplete T types
template <typename T, bool RequireAlignment = false>
class MaybeUniquePtr : detail::MaybeUniquePtrBase<T, RequireAlignment> {
  using detail::MaybeUniquePtrBase<T, RequireAlignment>::Data;

public:
  MaybeUniquePtr() noexcept = default;

  MaybeUniquePtr(T *Pointer, bool Owns = false) noexcept
      : detail::MaybeUniquePtrBase<T, RequireAlignment>(
            Pointer, Owns && Pointer != nullptr) {}

  MaybeUniquePtr(std::unique_ptr<T> &&Owner) noexcept
      : MaybeUniquePtr(Owner.release(), true) {}

  template <typename TT>
  MaybeUniquePtr(std::unique_ptr<TT> &&Owner) noexcept
      : MaybeUniquePtr(Owner.release(), true) {}

  MaybeUniquePtr(MaybeUniquePtr &&Other) noexcept
      : detail::MaybeUniquePtrBase<T, RequireAlignment>(
            std::exchange(Other.Data, {})) {}

  void swap(MaybeUniquePtr &Other) noexcept { std::swap(Data, Other, Data); }

  friend void swap(MaybeUniquePtr &LHS, MaybeUniquePtr &RHS) noexcept {
    LHS.swap(RHS);
  }

  MaybeUniquePtr &operator=(MaybeUniquePtr &&Other) noexcept {
    swap(Other);
    return *this;
  }

  MaybeUniquePtr &operator=(std::unique_ptr<T> &&Owner) noexcept {
    if (owns()) {
      delete Data.getPointer();
    }
    Data = {Owner.release(), true};
    return *this;
  }

  template <typename TT>
  MaybeUniquePtr &operator=(std::unique_ptr<TT> &&Owner) noexcept {
    if (owns()) {
      delete Data.getPointer();
    }
    Data = {Owner.release(), true};
    return *this;
  }

  MaybeUniquePtr(const MaybeUniquePtr &) = delete;
  MaybeUniquePtr &operator=(const MaybeUniquePtr &) = delete;

  ~MaybeUniquePtr() {
    if (owns()) {
      delete Data.getPointer();
      Data = {};
    }
  }

  [[nodiscard]] T *get() noexcept { return Data.getPointer(); }
  [[nodiscard]] const T *get() const noexcept { return Data.getPointer(); }

  [[nodiscard]] T *operator->() noexcept { return get(); }
  [[nodiscard]] const T *operator->() const noexcept { return get(); }

  [[nodiscard]] T &operator*() noexcept {
    assert(get() != nullptr);
    return *get();
  }
  [[nodiscard]] const T &operator*() const noexcept {
    assert(get() != nullptr);
    return *get();
  }

  T *release() noexcept {
    Data.setInt(false);
    return Data.getPointer();
  }

  void reset() noexcept {
    if (owns()) {
      delete Data.getPointer();
    }
    Data = {};
  }

  [[nodiscard]] bool owns() const noexcept {
    return Data.getInt() && Data.getPointer();
  }

  friend bool operator==(const MaybeUniquePtr &LHS,
                         const MaybeUniquePtr &RHS) noexcept {
    return LHS.Data.getPointer() == RHS.Data.getPointer();
  }
  friend bool operator!=(const MaybeUniquePtr &LHS,
                         const MaybeUniquePtr &RHS) noexcept {
    return !(LHS == RHS);
  }

  friend bool operator==(const MaybeUniquePtr &LHS, const T *RHS) noexcept {
    return LHS.Data.getPointer() == RHS;
  }
  friend bool operator!=(const MaybeUniquePtr &LHS, const T *RHS) noexcept {
    return !(LHS == RHS);
  }

  friend bool operator==(const T *LHS, const MaybeUniquePtr &RHS) noexcept {
    return LHS == RHS.Data.getPointer();
  }
  friend bool operator!=(const T *LHS, const MaybeUniquePtr &RHS) noexcept {
    return !(LHS == RHS);
  }

  explicit operator bool() const noexcept {
    return Data.getPointer() != nullptr;
  }
};

} // namespace psr

#endif // PHASAR_UTILS_MAYBEUNIQUEPTR_H_
