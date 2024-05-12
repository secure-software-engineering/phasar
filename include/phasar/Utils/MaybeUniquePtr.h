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
#include "llvm/Support/PointerLikeTypeTraits.h"

#include <memory>
#include <type_traits>

namespace psr {

namespace detail {
template <typename T, bool RequireAlignment> class MaybeUniquePtrBase {
protected:
  struct PointerBoolPairFallback {
    T *Pointer = nullptr;
    bool Flag = false;

    /// Compatibility with llvm::PointerIntPair:
    [[nodiscard]] constexpr T *getPointer() const noexcept { return Pointer; }
    [[nodiscard]] constexpr bool getInt() const noexcept { return Flag; }
    constexpr void setInt(bool Flag) noexcept { this->Flag = Flag; }
  };

  std::conditional_t<(alignof(T) > 1), llvm::PointerIntPair<T *, 1, bool>,
                     PointerBoolPairFallback>
      Data{};

  constexpr MaybeUniquePtrBase(T *Ptr, bool Owns) noexcept : Data{Ptr, Owns} {}
  constexpr MaybeUniquePtrBase() noexcept = default;
};

template <typename T> class MaybeUniquePtrBase<T, true> {
  struct PointerTraits : llvm::PointerLikeTypeTraits<T *> {
    static constexpr int NumLowBitsAvailable = 1;
  };

protected:
  llvm::PointerIntPair<T *, 1, bool, PointerTraits> Data{};

  constexpr MaybeUniquePtrBase(T *Ptr, bool Owns) noexcept : Data{Ptr, Owns} {}
  constexpr MaybeUniquePtrBase() noexcept = default;
};
} // namespace detail

/// A smart-pointer, similar to std::unique_ptr, that can optionally own an
/// object.
///
/// \tparam T The pointee type
/// \tparam RequireAlignment If true, the datastructure only works if
/// alignof(T) > 1 holds. Enables incomplete T types
template <typename T, bool RequireAlignment = false>
class [[clang::trivial_abi]] MaybeUniquePtr
    : detail::MaybeUniquePtrBase<T, RequireAlignment> {
  using detail::MaybeUniquePtrBase<T, RequireAlignment>::Data;

public:
  constexpr MaybeUniquePtr() noexcept = default;

  constexpr MaybeUniquePtr(T *Pointer, bool Owns = false) noexcept
      : detail::MaybeUniquePtrBase<T, RequireAlignment>(Pointer,
                                                        Owns && Pointer) {}

  constexpr MaybeUniquePtr(std::unique_ptr<T> &&Owner) noexcept
      : MaybeUniquePtr(Owner.release(), true) {}

  template <typename TT,
            typename = std::enable_if_t<!std::is_same_v<T, TT> &&
                                        std::is_convertible_v<TT *, T *>>>
  constexpr MaybeUniquePtr(std::unique_ptr<TT> &&Owner) noexcept
      : MaybeUniquePtr(Owner.release(), true) {}

  constexpr MaybeUniquePtr(MaybeUniquePtr &&Other) noexcept
      : detail::MaybeUniquePtrBase<T, RequireAlignment>(std::move(Other)) {
    Other.Data = {};
  }

  constexpr void swap(MaybeUniquePtr &Other) noexcept {
    std::swap(Data, Other.Data);
  }

  constexpr friend void swap(MaybeUniquePtr &LHS,
                             MaybeUniquePtr &RHS) noexcept {
    LHS.swap(RHS);
  }

  constexpr MaybeUniquePtr &operator=(MaybeUniquePtr &&Other) noexcept {
    swap(Other);
    return *this;
  }

  constexpr MaybeUniquePtr &operator=(std::unique_ptr<T> &&Owner) noexcept {
    if (owns()) {
      delete Data.getPointer();
    }
    auto *Ptr = Owner.release();
    Data = {Ptr, Ptr != nullptr};
    return *this;
  }

  template <typename TT,
            typename = std::enable_if_t<!std::is_same_v<T, TT> &&
                                        std::is_convertible_v<TT *, T *>>>
  constexpr MaybeUniquePtr &operator=(std::unique_ptr<TT> &&Owner) noexcept {
    if (owns()) {
      delete Data.getPointer();
    }
    auto *Ptr = Owner.release();
    Data = {Ptr, Ptr != nullptr};
    return *this;
  }

  MaybeUniquePtr(const MaybeUniquePtr &) = delete;
  MaybeUniquePtr &operator=(const MaybeUniquePtr &) = delete;

#if __cplusplus >= 202002L
  constexpr
#endif
      ~MaybeUniquePtr() {
    if (owns()) {
      delete Data.getPointer();
      Data = {};
    }
  }

  [[nodiscard]] constexpr T *get() const noexcept { return Data.getPointer(); }

  [[nodiscard]] constexpr T *operator->() const noexcept { return get(); }

  [[nodiscard]] constexpr T &operator*() const noexcept {
    assert(get() != nullptr);
    return *get();
  }

  constexpr T *release() noexcept {
    Data.setInt(false);
    return Data.getPointer();
  }

  constexpr void reset() noexcept {
    if (owns()) {
      delete Data.getPointer();
    }
    Data = {};
  }

  template <typename TT>
  constexpr void reset(MaybeUniquePtr<TT> &&Other) noexcept {
    *this = std::move(Other);
  }

  template <typename TT>
  constexpr void reset(std::unique_ptr<TT> &&Other) noexcept {
    *this = std::move(Other);
  }

  [[nodiscard]] constexpr bool owns() const noexcept {
    assert(Data.getPointer() || !Data.getInt());
    return Data.getInt();
  }

  constexpr friend bool operator==(const MaybeUniquePtr &LHS,
                                   const MaybeUniquePtr &RHS) noexcept {
    return LHS.Data.getPointer() == RHS.Data.getPointer();
  }
  constexpr friend bool operator!=(const MaybeUniquePtr &LHS,
                                   const MaybeUniquePtr &RHS) noexcept {
    return !(LHS == RHS);
  }

  constexpr friend bool operator==(const MaybeUniquePtr &LHS,
                                   const T *RHS) noexcept {
    return LHS.Data.getPointer() == RHS;
  }
  constexpr friend bool operator!=(const MaybeUniquePtr &LHS,
                                   const T *RHS) noexcept {
    return !(LHS == RHS);
  }

  constexpr friend bool operator==(const T *LHS,
                                   const MaybeUniquePtr &RHS) noexcept {
    return LHS == RHS.Data.getPointer();
  }
  constexpr friend bool operator!=(const T *LHS,
                                   const MaybeUniquePtr &RHS) noexcept {
    return !(LHS == RHS);
  }

  constexpr explicit operator bool() const noexcept {
    return Data.getPointer() != nullptr;
  }
};
} // namespace psr

#endif // PHASAR_UTILS_MAYBEUNIQUEPTR_H_
