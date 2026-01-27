#ifndef PHASAR_UTILS_POINTERUTILS_H
#define PHASAR_UTILS_POINTERUTILS_H

#include "phasar/Utils/BoxedPointer.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/ADT/IntrusiveRefCntPtr.h"

#include <memory>
#include <type_traits>

namespace psr {

/// A simple helper function to get a raw pointer from an arbitrary pointer type
/// in generic code. This overload set is extendable.

template <typename T>
  requires(!std::is_pointer_v<T>)
[[nodiscard]] constexpr T *getPointerFrom(T &Ref) noexcept {
  return std::addressof(Ref);
}
template <typename T>
  requires(!std::is_pointer_v<T>)
[[nodiscard]] constexpr const T *getPointerFrom(const T &Ref) noexcept {
  return std::addressof(Ref);
}
template <typename T>
  requires(!std::is_pointer_v<T>)
constexpr T *getPointerFrom(T &&Ref) noexcept = delete;

template <typename T> [[nodiscard]] T *getPointerFrom(T *Ptr) noexcept {
  return Ptr;
}
template <typename T>
[[nodiscard]] constexpr T *
getPointerFrom(const std::unique_ptr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
[[nodiscard]] constexpr T *getPointerFrom(std::unique_ptr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
constexpr T *getPointerFrom(std::unique_ptr<T> &&Ptr) noexcept = delete;

template <typename T, bool RequireAlignment>
[[nodiscard]] constexpr T *
getPointerFrom(const MaybeUniquePtr<T, RequireAlignment> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T, bool RequireAlignment>
[[nodiscard]] constexpr T *
getPointerFrom(MaybeUniquePtr<T, RequireAlignment> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T, bool RequireAlignment>
[[nodiscard]] constexpr T *
getPointerFrom(MaybeUniquePtr<T, RequireAlignment> &&Ptr) noexcept = delete;

template <typename T>
[[nodiscard]] constexpr T *
getPointerFrom(const std::shared_ptr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
[[nodiscard]] constexpr T *getPointerFrom(std::shared_ptr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
constexpr T *getPointerFrom(std::shared_ptr<T> &&Ptr) noexcept = delete;

template <typename T>
[[nodiscard]] constexpr T *
getPointerFrom(const llvm::IntrusiveRefCntPtr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
[[nodiscard]] constexpr T *
getPointerFrom(llvm::IntrusiveRefCntPtr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
[[nodiscard]] constexpr T *
getPointerFrom(llvm::IntrusiveRefCntPtr<T> &&Ptr) noexcept;

template <typename T>
[[nodiscard]] constexpr BoxedPtr<T> getPointerFrom(BoxedPtr<T> Ptr) noexcept {
  return Ptr;
}
template <typename T>
[[nodiscard]] constexpr BoxedConstPtr<T>
getPointerFrom(BoxedConstPtr<T> Ptr) noexcept {
  return Ptr;
}

static_assert(
    std::is_same_v<int *, decltype(getPointerFrom(
                              std::declval<MaybeUniquePtr<int> &>()))>);

static_assert(
    std::is_same_v<BoxedPtr<int>,
                   decltype(getPointerFrom(std::declval<BoxedPtr<int>>()))>);

} // namespace psr

#endif // PHASAR_UTILS_POINTERUTILS_H
