/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOINFO_H_
#define PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOINFO_H_

#include "phasar/PhasarLLVM/Pointer/PointsToInfo.h"

namespace llvm {
class Function;
class Instruction;
class Value;
} // namespace llvm

namespace psr {

class LLVMPointsToInfo
    : public PointsToInfo<const llvm::Value *, const llvm::Instruction *> {
public:
  using PointsToInfo::AllocationSiteSetPtrTy;
  using PointsToInfo::PointsToSetPtrTy;
  using PointsToInfo::PointsToSetTy;

  ~LLVMPointsToInfo() override = default;

  static const llvm::Function *retrieveFunction(const llvm::Value *V);
};

} // namespace psr

#endif
