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

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"

#include "phasar/PhasarLLVM/ControlFlow/CFG.h"
#include "phasar/Utils/LLVMIRToSrc.h"

#include "nlohmann/json.hpp"

namespace psr {

class LLVMBasedCFG
    : public virtual CFG<const llvm::Instruction *, const llvm::Function *> {
public:
  LLVMBasedCFG(bool IgnoreDbgInstructions = true)
      : IgnoreDbgInstructions(IgnoreDbgInstructions) {}

  ~LLVMBasedCFG() override = default;

  [[nodiscard]] const llvm::Function *
  getFunctionOf(const llvm::Instruction *Inst) const override;

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

  [[nodiscard]] bool isCallSite(const llvm::Instruction *Inst) const override;

  [[nodiscard]] bool isExitInst(const llvm::Instruction *Inst) const override;

  [[nodiscard]] bool isStartPoint(const llvm::Instruction *Inst) const override;

  [[nodiscard]] bool isFieldLoad(const llvm::Instruction *Inst) const override;

  [[nodiscard]] bool isFieldStore(const llvm::Instruction *Inst) const override;

  [[nodiscard]] bool
  isFallThroughSuccessor(const llvm::Instruction *Inst,
                         const llvm::Instruction *Succ) const override;

  [[nodiscard]] bool
  isBranchTarget(const llvm::Instruction *Inst,
                 const llvm::Instruction *Succ) const override;

  [[nodiscard]] bool
  isHeapAllocatingFunction(const llvm::Function *Fun) const override;

  [[nodiscard]] bool
  isSpecialMemberFunction(const llvm::Function *Fun) const override;

  [[nodiscard]] SpecialMemberFunctionType
  getSpecialMemberFunctionType(const llvm::Function *Fun) const override;

  [[nodiscard]] std::string
  getStatementId(const llvm::Instruction *Inst) const override;

  [[nodiscard]] std::string
  getFunctionName(const llvm::Function *Fun) const override;

  [[nodiscard]] std::string
  getDemangledFunctionName(const llvm::Function *Fun) const override;

  void print(const llvm::Function *Fun,
             llvm::raw_ostream &OS = llvm::outs()) const override;

  [[nodiscard]] nlohmann::json
  getAsJson(const llvm::Function *Fun) const override;

  [[nodiscard]] nlohmann::json exportCFGAsJson(const llvm::Function *F) const;

  [[nodiscard]] nlohmann::json
  exportCFGAsSourceCodeJson(const llvm::Function *F) const;

protected:
  // Ignores debug instructions in control flow if set to true.
  const bool IgnoreDbgInstructions;

  struct SourceCodeInfoWithIR : public SourceCodeInfo {
    std::string IR;
  };

  friend void from_json(const nlohmann::json &J, // NOLINT
                        SourceCodeInfoWithIR &Info);
  friend void to_json(nlohmann::json &J, // NOLINT
                      const SourceCodeInfoWithIR &Info);

  /// Used by export(I)CFGAsJson
  static SourceCodeInfoWithIR
  getFirstNonEmpty(llvm::BasicBlock::const_iterator &It,
                   llvm::BasicBlock::const_iterator End);
  static SourceCodeInfoWithIR getFirstNonEmpty(const llvm::BasicBlock *BB);
};

} // namespace psr

#endif
