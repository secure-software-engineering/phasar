/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMFIELDALIASSET_H
#define PHASAR_PHASARLLVM_POINTER_LLVMFIELDALIASSET_H

#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Pointer/AliasAnalysisType.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DataLayout.h"

#include <cstddef>
#include <functional>
#include <memory>

namespace psr {

class LLVMBasePointerAliasSet {
public:
  using v_t = typename LLVMAliasInfoRef::v_t;
  using n_t = typename LLVMAliasInfoRef::n_t;
  using AliasSetTy = LLVMAliasInfoRef::AliasSetTy;
  using AliasSetPtrTy = std::unique_ptr<AliasSetTy>;

  constexpr LLVMBasePointerAliasSet(LLVMAliasInfoRef AS) noexcept : AS(AS) {}

  [[nodiscard]] bool isInterProcedural() const noexcept {
    return AS.isInterProcedural();
  }

  [[nodiscard]] AliasAnalysisType getAliasAnalysisType() const noexcept {
    return AS.getAliasAnalysisType();
  }

  [[nodiscard]] static const llvm::Value *
  getBasePointer(const llvm::Value *Pointer);

  [[nodiscard]] AliasResult alias(v_t Pointer1, v_t Pointer2,
                                  n_t AtInstruction = {}) const {
    return AS.alias(Pointer1, Pointer2, AtInstruction);
  }

  [[nodiscard]] AliasSetPtrTy getAliasSet(v_t Pointer,
                                          n_t AtInstruction = {}) const;

private:
  LLVMAliasInfoRef AS;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_LLVMFIELDALIASSET_H
