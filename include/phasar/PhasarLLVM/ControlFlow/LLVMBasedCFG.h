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

#ifndef SRC_ANALYSIS_ICFG_LLVMBASEDCFG_H_
#define SRC_ANALYSIS_ICFG_LLVMBASEDCFG_H_

#include <iostream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <phasar/Config/Configuration.h>
#include <phasar/PhasarLLVM/ControlFlow/CFG.h>
#include <set>
#include <string>
#include <vector>
using namespace std;
namespace psr {

class LLVMBasedCFG
    : public CFG<const llvm::Instruction *, const llvm::Function *> {
public:
  LLVMBasedCFG() = default;

  virtual ~LLVMBasedCFG() = default;

  virtual const llvm::Function *
  getMethodOf(const llvm::Instruction *stmt) override;

  virtual vector<const llvm::Instruction *>
  getPredsOf(const llvm::Instruction *stmt) override;

  virtual vector<const llvm::Instruction *>
  getSuccsOf(const llvm::Instruction *stmt) override;

  virtual vector<pair<const llvm::Instruction *, const llvm::Instruction *>>
  getAllControlFlowEdges(const llvm::Function *fun) override;

  virtual vector<const llvm::Instruction *>
  getAllInstructionsOf(const llvm::Function *fun) override;

  virtual bool isExitStmt(const llvm::Instruction *stmt) override;

  virtual bool isStartPoint(const llvm::Instruction *stmt) override;

  virtual bool isFallThroughSuccessor(const llvm::Instruction *stmt,
                                      const llvm::Instruction *succ) override;

  virtual bool isBranchTarget(const llvm::Instruction *stmt,
                              const llvm::Instruction *succ) override;

  virtual string getStatementId(const llvm::Instruction *stmt) override;

  virtual string getMethodName(const llvm::Function *fun) override;
};

} // namespace psr

#endif /* SRC_ANALYSIS_ICFG_LLVMBASEDCFG_HH_ */
