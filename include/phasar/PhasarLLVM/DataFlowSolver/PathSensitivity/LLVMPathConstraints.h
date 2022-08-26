/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_LLVMPATHCONSTRAINTS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_LLVMPATHCONSTRAINTS_H

#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/ADT/SmallVector.h"

#include "z3++.h"

#include <optional>

namespace llvm {
class Value;
class Instruction;
class AllocaInst;
class LoadInst;
class GetElementPtrInst;
class PHINode;
class BranchInst;
class CmpInst;
class BinaryOperator;
class CallBase;
} // namespace llvm

namespace psr {
class LLVMPathConstraints {
public:
  struct ConstraintAndVariables {
    z3::expr Constraint;
    llvm::SmallVector<const llvm::Value *, 4> Variables;
  };

  explicit LLVMPathConstraints(z3::context *Z3Ctx = nullptr,
                               bool IgnoreDebugInstructions = true);

  z3::context &getContext() noexcept { return *Z3Ctx; }
  const z3::context &getContext() const noexcept { return *Z3Ctx; }

  std::optional<z3::expr> getConstraintFromEdge(const llvm::Instruction *Curr,
                                                const llvm::Instruction *Succ);

  std::optional<ConstraintAndVariables>
  getConstraintAndVariablesFromEdge(const llvm::Instruction *Curr,
                                    const llvm::Instruction *Succ);

private:
  [[nodiscard]] std::optional<ConstraintAndVariables>
  internalGetConstraintAndVariablesFromEdge(const llvm::Instruction *From,
                                            const llvm::Instruction *To);

  /// Allocas are the most basic building blocks and represent a leaf value.
  [[nodiscard]] ConstraintAndVariables
  getAllocaInstAsZ3(const llvm::AllocaInst *Alloca);

  /// Load instrutions may also represent leafs.
  [[nodiscard]] ConstraintAndVariables
  getLoadInstAsZ3(const llvm::LoadInst *Load);

  /// GEP instructions may also represent leafs.
  [[nodiscard]] ConstraintAndVariables
  getGEPInstAsZ3(const llvm::GetElementPtrInst *GEP);

  [[nodiscard]] ConstraintAndVariables
  handlePhiInstruction(const llvm::PHINode *Phi);

  [[nodiscard]] ConstraintAndVariables
  handleCondBrInst(const llvm::BranchInst *Br, const llvm::Instruction *Succ);

  [[nodiscard]] ConstraintAndVariables handleCmpInst(const llvm::CmpInst *Cmp);

  [[nodiscard]] ConstraintAndVariables
  handleBinaryOperator(const llvm::BinaryOperator *BinOp);

  [[nodiscard]] ConstraintAndVariables getLiteralAsZ3(const llvm::Value *V);

  [[nodiscard]] ConstraintAndVariables
  getFunctionCallAsZ3(const llvm::CallBase *CallSite);

  friend class LLVMPathConstraints;
  friend class SizeGuardCheck;
  friend class LoopGuardCheck;

  MaybeUniquePtr<z3::context> Z3Ctx;
  std::unordered_map<const llvm::Value *, ConstraintAndVariables> Z3Expr;
  bool IgnoreDebugInstructions;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_LLVMPATHCONSTRAINTS_H
