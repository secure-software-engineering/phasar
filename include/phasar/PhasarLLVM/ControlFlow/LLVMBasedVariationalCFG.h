/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDVARIATIONALCFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDVARIATIONALCFG_H_

#include <utility>

#include <z3++.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>
#include <phasar/PhasarLLVM/ControlFlow/VariationalCFG.h>

namespace llvm {
class Function;
class Instruction;
class BranchInst;
class BinaryOperator;
class LoadInst;
class GlobalVariable;
} // namespace llvm

namespace psr {

class LLVMBasedVariationalCFG
    : public virtual LLVMBasedCFG,
      public virtual VariationalCFG<const llvm::Instruction *,
                                    const llvm::Function *, z3::expr> {
private:
  // TODO: check if those variables need to be mutable, i.e. the z3
  // related member functions need to be const.
  mutable z3::context CTX;
  mutable std::unordered_map<std::string, z3::expr> PPVariables;

  bool isPPBranchNode(const llvm::BranchInst *BI) const;

  bool isPPBranchNode(const llvm::BranchInst *BI, z3::expr &Cond) const;

  z3::expr inferCondition(const llvm::CmpInst *Cmp) const;

  z3::expr createExpression(const llvm::Value *V) const;

  z3::expr createBinOp(const llvm::BinaryOperator *OP) const;

  z3::expr createVariableOrGlobal(const llvm::LoadInst *LI) const;

  z3::expr createConstant(const llvm::Constant *C) const;

  z3::expr compareBoolAndInt(z3::expr XBool, z3::expr XInt,
                             bool ForEquality) const;

  bool isPPVariable(const llvm::GlobalVariable *G, std::string &Name) const;

public:
  LLVMBasedVariationalCFG() = default;

  ~LLVMBasedVariationalCFG() override = default;

  std::vector<std::pair<const llvm::Instruction *, z3::expr>>
  getSuccsOfWithPPConstraints(const llvm::Instruction *Stmt) const override;

  bool isPPBranchTarget(const llvm::Instruction *Stmt,
                        const llvm::Instruction *Succ) const override;

  z3::expr getPPConstraintOrTrue(const llvm::Instruction *Stmt,
                                 const llvm::Instruction *Succ) const override;

  z3::expr getTrueConstraint() const override;

  z3::context &getContext() const;
};

} // namespace psr

#endif
