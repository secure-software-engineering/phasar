/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_LIBRARY_SUMMARY_H
#define PHASAR_UTILS_LIBRARY_SUMMARY_H

#include "llvm/ADT/StringRef.h"

namespace psr {

/// Checks, whether a function with the given (mangled) name is known to be a
/// heap-allocating function, similar to malloc.
[[nodiscard]] bool isHeapAllocatingFunction(llvm::StringRef FName) noexcept;

/// Checks whether a function with the given (mangled) name is known to always
/// return the same pointer
[[nodiscard]] bool isSingletonReturningFunction(llvm::StringRef FName) noexcept;

} // namespace psr

#endif // PHASAR_UTILS_LIBRARY_SUMMARY_H
