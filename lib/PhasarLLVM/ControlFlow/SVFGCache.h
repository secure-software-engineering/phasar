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

#include "phasar/ControlFlow/SparseCFGProvider.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMControlFlow.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/ADT/DenseMap.h"
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
  using n_t = const llvm::Instruction *;
  std::unordered_map<std::pair<f_t, v_t>, SparseLLVMBasedCFG, FVHasher> Cache{};
  llvm::DenseMap<std::pair<n_t, v_t>, n_t> SameOrNextUserCache{};

  [[nodiscard]] LLVM_LIBRARY_VISIBILITY const SparseLLVMBasedCFG &
  getOrCreate(const LLVMBasedCFG &CFG, const llvm::Function *Fun,
              const llvm::Value *Val, LLVMAliasInfoRef AliasAnalysis);

  [[nodiscard]] n_t advanceToNextUser(n_t Succ, const auto &Fact,
                                      LLVMAliasInfoRef AliasAnalysis) {
    using psr::valueOf;

    // XXX: Measure, whether caching actually helps here...
    // XXX: Make thread-safe:

    auto [It, Inserted] =
        SameOrNextUserCache.try_emplace(std::pair{Succ, valueOf(Fact)});
    if (Inserted) {
      It->second =
          SparseLLVMControlFlow::advanceToNextUser(Succ, Fact, AliasAnalysis);
    }
    return It->second;
  }
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_SPARSECFGCACHE_H
