/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDCFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDCFG_H_

#include "phasar/PhasarLLVM/ControlFlow/CFGBase.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"

namespace llvm {
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class ProjectIRDB;
class LLVMBasedBackwardCFG;

class LLVMBasedBackwardCFG
    : public detail::LLVMBasedCFGImpl<LLVMBasedBackwardCFG> {
  friend CFGBase;

  using base_t = detail::LLVMBasedCFGImpl<LLVMBasedBackwardCFG>;

public:
  LLVMBasedBackwardCFG(bool IgnoreDbgInstructions = true) noexcept;
  LLVMBasedBackwardCFG(const ProjectIRDB &IRDB,
                       bool IgnoreDbgInstructions = true);

private:
  [[nodiscard]] f_t getFunctionOfImpl(n_t Inst) const noexcept;
  [[nodiscard]] llvm::SmallVector<n_t, 2> getPredsOfImpl(n_t Inst) const;
  [[nodiscard]] llvm::SmallVector<n_t, 2> getSuccsOfImpl(n_t Inst) const;
  [[nodiscard]] std::vector<std::pair<n_t, n_t>>
  getAllControlFlowEdgesImpl(f_t Fun) const;

  [[nodiscard]] llvm::SmallVector<n_t, 2> getStartPointsOfImpl(f_t Fun) const;
  [[nodiscard]] llvm::SmallVector<n_t, 2> getExitPointsOfImpl(f_t Fun) const;

  [[nodiscard]] bool isExitInstImpl(n_t Inst) const noexcept;
  [[nodiscard]] bool isStartPointImpl(n_t Inst) const noexcept;

  [[nodiscard]] bool isFallThroughSuccessorImpl(n_t Inst,
                                                n_t Succ) const noexcept;
  [[nodiscard]] bool isBranchTargetImpl(n_t Inst, n_t Succ) const noexcept;

  llvm::DenseMap<const llvm::Function *, const llvm::Instruction *>
      BackwardRets;
  llvm::DenseMap<const llvm::Instruction *, const llvm::Function *>
      BackwardRetToFunction;
};

} // namespace psr

#endif
