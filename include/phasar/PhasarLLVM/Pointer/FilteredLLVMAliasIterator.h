/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_FILTEREDLLVMALIASITERATOR_H
#define PHASAR_PHASARLLVM_POINTER_FILTEREDLLVMALIASITERATOR_H

#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/ADT/STLFunctionalExtras.h"

namespace psr {
class FilteredLLVMAliasIterator {
public:
  using n_t = const llvm::Instruction *;
  using v_t = const llvm::Value *;

  constexpr FilteredLLVMAliasIterator(LLVMAliasIteratorRef Underlying) noexcept
      : Underlying(Underlying) {}

  void forallAliasesOf(const llvm::Value *V, const llvm::Function *Fun,
                       llvm::function_ref<void(const llvm::Value *)> WithAlias);
  void forallAliasesOf(const llvm::Value *V, const llvm::Instruction *At,
                       llvm::function_ref<void(const llvm::Value *)> WithAlias);

  [[nodiscard]] LLVMAliasIteratorRef getUnderlying() const noexcept {
    return Underlying;
  }

private:
  LLVMAliasIteratorRef Underlying;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_FILTEREDLLVMALIASITERATOR_H
