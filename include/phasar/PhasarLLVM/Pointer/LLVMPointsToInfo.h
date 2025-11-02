/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOINFO_H
#define PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOINFO_H

#include "phasar/Pointer/PointsToIterator.h"

namespace llvm {
class Instruction;
class Value;
} // namespace llvm

namespace psr {

using LLVMPointsToIteratorRef =
    PointsToIteratorRef<const llvm::Value *, const llvm::Value *,
                        const llvm::Instruction *>;

using LLVMPointsToIterator =
    PointsToIterator<const llvm::Value *, const llvm::Value *,
                     const llvm::Instruction *>;
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOINFO_H
