/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_SPARSELLVMBASEDCFGPROVIDER_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_SPARSELLVMBASEDCFGPROVIDER_H

#include "phasar/ControlFlow/SparseCFGProvider.h"

namespace llvm {
class Function;
class Value;
} // namespace llvm

namespace psr {

template <typename Derived>
using SparseLLVMBasedCFGProvider =
    SparseCFGProvider<Derived, const llvm::Function *, const llvm::Value *>;

[[nodiscard]] constexpr const llvm::Value *
valueOf(const llvm::Value *V) noexcept {
  return V;
}

} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_SPARSELLVMBASEDCFGPROVIDER_H
