/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_POINTERUTILS_H
#define PHASAR_UTILS_POINTERUTILS_H

#include "llvm/ADT/IntrusiveRefCntPtr.h"

#include <memory>

namespace psr {

/// A simple helper function to get a raw pointer from an arbitrary pointer type
/// in generic code. This overload set is extendable.

template <typename T> T *getPointerFrom(T *Ptr) noexcept { return Ptr; }
template <typename T>
constexpr T *getPointerFrom(const std::unique_ptr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
constexpr T *getPointerFrom(const std::shared_ptr<T> &Ptr) noexcept {
  return Ptr.get();
}
template <typename T>
constexpr T *getPointerFrom(const llvm::IntrusiveRefCntPtr<T> &Ptr) noexcept {
  return Ptr.get();
}

} // namespace psr

#endif // PHASAR_UTILS_POINTERUTILS_H
