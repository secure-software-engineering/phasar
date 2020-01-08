#pragma once
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>
#include <phasar/PhasarLLVM/ControlFlow/VariationalCFG.h>
#include <z3++.h>
namespace llvm {
class Function;
class Instruction;
} // namespace llvm

namespace psr {
class LLVMBasedVariationalCFG
    : public virtual LLVMBasedCFG,
      public virtual VariationalCFG<const llvm::Instruction *,
                                    const llvm::Function *, z3::expr> {
  std::unique_ptr<z3::context> ctx;
  bool isPPBranchNode(const llvm::BranchInst *T) const;
  bool isPPBranchNode(const llvm::BranchInst *T, z3::expr &cond) const;
  z3::expr inferCondition(const llvm::CmpInst *cmp) const;
  z3::expr createExpression(const llvm::Value *val) const;
  z3::expr createBinOp(const llvm::BinaryOperator *val) const;
  z3::expr createVariableOrGlobal(const llvm::LoadInst *val) const;
  z3::expr createGEP(const llvm::GetElementPtrInst *val) const;

public:
  LLVMBasedVariationalCFG();
  virtual ~LLVMBasedVariationalCFG() = default;
  std::vector<std::tuple<const llvm::Instruction *, z3::expr>>
  getSuccsOfWithCond(const llvm::Instruction *stmt) override;
  bool isPPBranchTarget(const llvm::Instruction *stmt,
                        const llvm::Instruction *succ) const override;
  bool isPPBranchTarget(const llvm::Instruction *stmt,
                        const llvm::Instruction *succ,
                        z3::expr &condition) const override;
  z3::expr getTrueCondition() const override;
};
} // namespace psr