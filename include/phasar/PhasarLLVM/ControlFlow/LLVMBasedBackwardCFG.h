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

#include <phasar/PhasarLLVM/ControlFlow/CFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>

namespace llvm {
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class LLVMBasedBackwardCFG
    : public virtual CFG<const llvm::Instruction *, const llvm::Function *> {
private:
  LLVMBasedCFG ForwardCFG;

public:
  LLVMBasedBackwardCFG() = default;

  virtual ~LLVMBasedBackwardCFG() = default;

  const llvm::Function *
  getFunctionOf(const llvm::Instruction *stmt) const override;

  std::vector<const llvm::Instruction *>
  getPredsOf(const llvm::Instruction *stmt) const override;

  std::vector<const llvm::Instruction *>
  getSuccsOf(const llvm::Instruction *stmt) const override;

  std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
  getAllControlFlowEdges(const llvm::Function *fun) const override;

  std::vector<const llvm::Instruction *>
  getAllInstructionsOf(const llvm::Function *fun) const override;

  bool isExitStmt(const llvm::Instruction *stmt) const override;

  bool isStartPoint(const llvm::Instruction *stmt) const override;

  bool isFieldLoad(const llvm::Instruction *stmt) const override;

  bool isFieldStore(const llvm::Instruction *stmt) const override;

  bool isFallThroughSuccessor(const llvm::Instruction *stmt,
                              const llvm::Instruction *succ) const override;

  bool isBranchTarget(const llvm::Instruction *stmt,
                      const llvm::Instruction *succ) const override;

  std::string getFunctionName(const llvm::Function *fun) const override;

  std::string getStatementId(const llvm::Instruction *stmt) const override;

  void print(const llvm::Function *F, std::ostream &OS) const override;

  nlohmann::json getAsJson(const llvm::Function *F) const override;
};
} // namespace psr

#endif
