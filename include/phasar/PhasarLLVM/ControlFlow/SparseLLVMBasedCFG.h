/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_SPARSELLVMBASEDCFG_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_SPARSELLVMBASEDCFG_H

#include "phasar/ControlFlow/SparseCFGBase.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"

#include "llvm/ADT/DenseMap.h"

namespace psr {

class SparseLLVMBasedCFG;

template <> struct CFGTraits<SparseLLVMBasedCFG> : CFGTraits<LLVMBasedCFG> {
  using v_t = const llvm::Value *;
};

class SparseLLVMBasedCFG : public LLVMBasedCFG,
                           public SparseCFGBase<SparseLLVMBasedCFG> {
  friend struct SVFGCache;
  friend SparseCFGBase<SparseLLVMBasedCFG>;

public:
  using vgraph_t =
      llvm::SmallDenseMap<const llvm::Instruction *, const llvm::Instruction *>;

  SparseLLVMBasedCFG() noexcept = default;
  SparseLLVMBasedCFG(
      llvm::SmallDenseMap<const llvm::Instruction *, const llvm::Instruction *>
          &&VGraph) noexcept
      : VGraph(std::move(VGraph)) {}

private:
  [[nodiscard]] n_t nextUserOrNullImpl(n_t FromInstruction) const {
    return VGraph.lookup(FromInstruction);
  }

  vgraph_t VGraph;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_SPARSELLVMBASEDCFG_H
