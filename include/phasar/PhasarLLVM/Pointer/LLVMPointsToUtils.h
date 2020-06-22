/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOUTILS_H_
#define PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOUTILS_H_

#include <set>

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

namespace llvm {
class Value;
} // namespace llvm

namespace psr {

/**
 * @brief Returns true if the given pointer is an interesting pointer,
 *        i.e. not a constant null pointer.
 */
inline bool isInterestingPointer(const llvm::Value *V) {
  return V->getType()->isPointerTy() &&
         !llvm::isa<llvm::ConstantPointerNull>(V);
}

extern const std::set<llvm::StringRef> HeapAllocatingFunctions;

} // namespace psr

#endif
