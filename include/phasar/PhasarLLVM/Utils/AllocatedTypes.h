/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_UTILS_ALLOCATEDTYPES_H
#define PHASAR_PHASARLLVM_UTILS_ALLOCATEDTYPES_H

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Module.h"

#include <vector>

namespace psr {
[[nodiscard]] std::vector<const llvm::DICompositeType *>
collectAllocatedTypes(const llvm::Module &Mod);
} // namespace psr

#endif // PHASAR_PHASARLLVM_UTILS_ALLOCATEDTYPES_H
