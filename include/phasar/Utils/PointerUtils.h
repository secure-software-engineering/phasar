#ifndef PHASAR_UTILS_POINTERUTILS_H
#define PHASAR_UTILS_POINTERUTILS_H

#include "llvm/ADT/IntrusiveRefCntPtr.h"

#include <memory>
#include <type_traits>

namespace psr {

/// A simple helper function to get a raw pointer from an arbitrary pointer type
/// in generic code. This overload set is extendable.

template <typename T>
constexpr std::enable_if_t<!std::is_pointer_v<T>, T *>
getPointerFrom(T &Ref) noexcept {
  return std::addressof(Ref);
}
template <typename T>
constexpr std::enable_if_t<!std::is_pointer_v<T>, const T *>
getPointerFrom(const T &Ref) noexcept {
  return std::addressof(Ref);
}
template <typename T>
constexpr std::enable_if_t<!std::is_pointer_v<T>, T *>
getPointerFrom(T &&Ref) noexcept = delete;

template <typename T> T *getPointerFrom(T *Ptr) noexcept { return Ptr; }
template <typename T>
constexpr T *getPointerFrom(const std::unique_ptr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
constexpr T *getPointerFrom(std::unique_ptr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
constexpr T *getPointerFrom(const std::shared_ptr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
constexpr T *getPointerFrom(std::shared_ptr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
constexpr T *getPointerFrom(const llvm::IntrusiveRefCntPtr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
constexpr T *getPointerFrom(llvm::IntrusiveRefCntPtr<T> &Ptr) noexcept {
  return Ptr.get();
}

} // namespace psr

#endif // PHASAR_UTILS_POINTERUTILS_H
