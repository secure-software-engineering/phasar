/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_SVFGCACHE_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_SVFGCACHE_H

#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFG.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/Compiler.h"

#include <unordered_map>

namespace psr {
struct FVHasher {
  auto operator()(std::pair<const llvm::Function *, const llvm::Value *> FV)
      const noexcept {
    return llvm::hash_value(FV);
  }
};

struct SVFGCache {
  using f_t = const llvm::Function *;
  using v_t = const llvm::Value *;
  std::unordered_map<std::pair<f_t, v_t>, SparseLLVMBasedCFG, FVHasher> Cache{};

  LLVM_LIBRARY_VISIBILITY const SparseLLVMBasedCFG &
  getOrCreate(const LLVMBasedCFG &CFG, const llvm::Function *Fun,
              const llvm::Value *Val);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_SPARSECFGCACHE_H
