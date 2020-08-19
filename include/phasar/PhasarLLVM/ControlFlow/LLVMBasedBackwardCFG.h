/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMBasedBackwardCFG.h
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDCFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDCFG_H_

#include <set>
#include <string>
#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"

namespace llvm {
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class LLVMBasedBackwardCFG : public LLVMBasedCFG {
public:
  LLVMBasedBackwardCFG() = default;

  ~LLVMBasedBackwardCFG() override = default;

  [[nodiscard]] std::vector<const llvm::Instruction *>
  getPredsOf(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] std::vector<const llvm::Instruction *>
  getSuccsOf(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] std::set<const llvm::Instruction *>
  getStartPointsOf(const llvm::Function *Fun) const override;

  [[nodiscard]] std::set<const llvm::Instruction *>
  getExitPointsOf(const llvm::Function *Fun) const override;

  [[nodiscard]] bool isExitStmt(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] bool isStartPoint(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] bool
  isFallThroughSuccessor(const llvm::Instruction *Inst,
                         const llvm::Instruction *succ) const override;

  [[nodiscard]] bool
  isBranchTarget(const llvm::Instruction *Stmt,
                 const llvm::Instruction *succ) const override;
};
} // namespace psr

#endif
