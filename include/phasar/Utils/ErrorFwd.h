/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_ERRORFWD_H
#define PHASAR_UTILS_ERRORFWD_H

#include "llvm/ADT/StringRef.h"

namespace psr {
[[noreturn, gnu::cold]] void
composeEFPureVirtualError(llvm::StringRef ConcreteEF, llvm::StringRef L);
[[noreturn, gnu::cold]] void joinEFPureVirtualError(llvm::StringRef ConcreteEF,
                                                    llvm::StringRef L);
} // namespace psr

#endif // PHASAR_UTILS_ERRORFWD_H
