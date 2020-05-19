/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"

namespace psr {

bool isInterestingPointer(const llvm::Value *V) {
  return V->getType()->isPointerTy() &&
         !llvm::isa<llvm::ConstantPointerNull>(V);
}

const std::set<llvm::StringRef> HeapAllocatingFunctions{
    "malloc", "calloc", "realloc", "_Znwm", "_Znam"};

} // namespace psr
