#pragma once

#include "phasar/Utils/Utilities.h"

#include "llvm/Support/Compiler.h"

#include <functional>

namespace psr {
template <typename T>
class [[gsl::Pointer(T)]] NonNullPtr : public std::reference_wrapper<T> {
public:
  constexpr NonNullPtr(std::nullptr_t) = delete;

  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr NonNullPtr(T *Ptr) noexcept
      : std::reference_wrapper<T>(psr::assertNotNull(Ptr)) {}

  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr NonNullPtr(
      std::reference_wrapper<T> RW) noexcept
      : std::reference_wrapper<T>(RW) {}

  LLVM_ATTRIBUTE_ALWAYS_INLINE explicit constexpr NonNullPtr(T &Ref) noexcept
      : std::reference_wrapper<T>(Ref) {}

  template <typename U>
    requires(!std::same_as<U, T> && std::convertible_to<U *, T *>)
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr NonNullPtr(
      NonNullPtr<U> Other) noexcept
      : std::reference_wrapper<T>(Other) {}

  [[nodiscard]] LLVM_ATTRIBUTE_ALWAYS_INLINE
      LLVM_ATTRIBUTE_RETURNS_NONNULL constexpr T *
      operator->() const noexcept {
    return std::addressof(this->get());
  }

  [[nodiscard]] LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr T &
  operator*() const noexcept {
    return this->get();
  }

  [[nodiscard]] LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr bool
  operator==(NonNullPtr Other) noexcept {
    return &this->get() == &Other.get();
  }
  [[nodiscard]] LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr bool
  operator==(const T *R) noexcept {
    return &this->get() == R;
  }
  [[nodiscard]] LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr bool
  operator==(std::nullptr_t) noexcept = delete; // NonNullPtr is never null

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, NonNullPtr RW) {
    return OS << &RW.get();
  }
  friend std::ostream &operator<<(std::ostream &OS, NonNullPtr RW) {
    return OS << &RW.get();
  }
};
} // namespace psr
