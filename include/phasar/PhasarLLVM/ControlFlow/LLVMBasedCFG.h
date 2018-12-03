/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMBasedCFG.h
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDCFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDCFG_H_

#include <set>
#include <string>
#include <vector>

#include <phasar/PhasarLLVM/ControlFlow/CFG.h>

namespace llvm {
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class LLVMBasedCFG
    : public virtual CFG<const llvm::Instruction *, const llvm::Function *> {
public:
  LLVMBasedCFG() = default;

  ~LLVMBasedCFG() override = default;

  const llvm::Function *getMethodOf(const llvm::Instruction *stmt) override;

  std::vector<const llvm::Instruction *>
  getPredsOf(const llvm::Instruction *stmt) override;

  std::vector<const llvm::Instruction *>
  getSuccsOf(const llvm::Instruction *stmt) override;

  std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
  getAllControlFlowEdges(const llvm::Function *fun) override;

  std::vector<const llvm::Instruction *>
  getAllInstructionsOf(const llvm::Function *fun) override;

  bool isExitStmt(const llvm::Instruction *stmt) override;

  bool isStartPoint(const llvm::Instruction *stmt) override;

  bool isFieldLoad(const llvm::Instruction *stmt) override;

  bool isFieldStore(const llvm::Instruction *stmt) override;

  bool isFallThroughSuccessor(const llvm::Instruction *stmt,
                              const llvm::Instruction *succ) override;

  bool isBranchTarget(const llvm::Instruction *stmt,
                      const llvm::Instruction *succ) override;

  std::string getStatementId(const llvm::Instruction *stmt) override;

  std::string getMethodName(const llvm::Function *fun) override;
};

} // namespace psr

#endif
