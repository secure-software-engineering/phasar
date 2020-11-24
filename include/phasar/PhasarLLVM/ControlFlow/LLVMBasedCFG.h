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

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/CFG.h"

namespace llvm {
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class LLVMBasedCFG
    : public virtual CFG<const llvm::Instruction *, const llvm::Function *> {
public:
  LLVMBasedCFG(bool IgnoreDbgInstructions = true)
      : IgnoreDbgInstructions(IgnoreDbgInstructions) {}

  ~LLVMBasedCFG() override = default;

  [[nodiscard]] const llvm::Function *
  getFunctionOf(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] std::vector<const llvm::Instruction *>
  getPredsOf(const llvm::Instruction *Inst) const override;

  [[nodiscard]] std::vector<const llvm::Instruction *>
  getSuccsOf(const llvm::Instruction *Inst) const override;

  [[nodiscard]] std::vector<
      std::pair<const llvm::Instruction *, const llvm::Instruction *>>
  getAllControlFlowEdges(const llvm::Function *Fun) const override;

  [[nodiscard]] std::vector<const llvm::Instruction *>
  getAllInstructionsOf(const llvm::Function *Fun) const override;

  [[nodiscard]] std::set<const llvm::Instruction *>
  getStartPointsOf(const llvm::Function *Fun) const override;

  [[nodiscard]] std::set<const llvm::Instruction *>
  getExitPointsOf(const llvm::Function *Fun) const override;

  [[nodiscard]] bool isCallStmt(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] bool isExitStmt(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] bool isStartPoint(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] bool isFieldLoad(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] bool isFieldStore(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] bool
  isFallThroughSuccessor(const llvm::Instruction *Stmt,
                         const llvm::Instruction *succ) const override;

  [[nodiscard]] bool
  isBranchTarget(const llvm::Instruction *Stmt,
                 const llvm::Instruction *succ) const override;

  [[nodiscard]] bool
  isHeapAllocatingFunction(const llvm::Function *Fun) const override;

  [[nodiscard]] bool
  isSpecialMemberFunction(const llvm::Function *Fun) const override;

  [[nodiscard]] SpecialMemberFunctionType
  getSpecialMemberFunctionType(const llvm::Function *Fun) const override;

  [[nodiscard]] std::string
  getStatementId(const llvm::Instruction *Stmt) const override;

  [[nodiscard]] std::string
  getFunctionName(const llvm::Function *Fun) const override;

  [[nodiscard]] std::string
  getDemangledFunctionName(const llvm::Function *Fun) const override;

  void print(const llvm::Function *Fun,
             std::ostream &OS = std::cout) const override;

  [[nodiscard]] nlohmann::json
  getAsJson(const llvm::Function *Fun) const override;

private:
  // Ignores debug instructions in control flow if set to true.
  const bool IgnoreDbgInstructions;
};

} // namespace psr

#endif
