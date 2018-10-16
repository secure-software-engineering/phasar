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

namespace llvm {
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class LLVMBasedBackwardCFG
    : public CFG<const llvm::Instruction *, const llvm::Function *> {
public:
  LLVMBasedBackwardCFG()=default;

  virtual ~LLVMBasedBackwardCFG()=default;

  virtual const llvm::Function *
  getMethodOf(const llvm::Instruction *stmt) override;

  virtual std::vector<const llvm::Instruction *>
  getPredsOf(const llvm::Instruction *stmt) override;

  virtual std::vector<const llvm::Instruction *>
  getSuccsOf(const llvm::Instruction *stmt) override;

  virtual std::vector<
      std::pair<const llvm::Instruction *, const llvm::Instruction *>>
  getAllControlFlowEdges(const llvm::Function *fun) override;

  virtual std::vector<const llvm::Instruction *>
  getAllInstructionsOf(const llvm::Function *fun) override;

  virtual bool isExitStmt(const llvm::Instruction *stmt) override;

  virtual bool isStartPoint(const llvm::Instruction *stmt) override;

  virtual bool isFallThroughSuccessor(const llvm::Instruction *stmt,
                                      const llvm::Instruction *succ) override;

  virtual bool isBranchTarget(const llvm::Instruction *stmt,
                              const llvm::Instruction *succ) override;

  virtual std::string getMethodName(const llvm::Function *fun) override;
  virtual std::string getStatementId(const llvm::Instruction *stmt) override;
};
} // namespace psr

#endif
