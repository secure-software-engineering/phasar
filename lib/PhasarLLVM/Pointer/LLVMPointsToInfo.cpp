/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"

using namespace std;
using namespace psr;

namespace psr {

llvm::Function *LLVMPointsToInfo::retrieveFunction(const llvm::Value *V) {
  const llvm::Function *Fun = nullptr;
  if (V) {
    if (auto Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
      Fun = Inst->getFunction();
    }
    if (auto BB = llvm::dyn_cast<llvm::BasicBlock>(V)) {
      Fun = BB->getParent();
    }
    if (auto Arg = llvm::dyn_cast<llvm::Argument>(V)) {
      Fun = Arg->getParent();
    }
  }
  return const_cast<llvm::Function *>(Fun);
}

} // namespace psr
