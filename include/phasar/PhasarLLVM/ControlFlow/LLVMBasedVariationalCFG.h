#pragma once
#include <phasar/PhasarLLVM/ControlFlow/VariationalCFG.h>
#include <z3++.h>
namespace llvm {
class Function;
class Instruction;
} // namespace llvm

namespace psr {
class LLVMBasedVariationalCFG
    : public virtual VariationalCFG<const llvm::Instruction *,
                                    const llvm::Function *, z3::expr> {
  z3::context ctx;
  bool isPPBranchNode(const llvm::BranchInst *T);
  bool isPPBranchNode(const llvm::BranchInst *T, z3::expr &cond);
  z3::expr inferCondition(const llvm::CMPInst cmp);

public:
  virtual ~LLVMBasedVariationalCFG() = default;
  std::vectorstd::tuple<const llvm::Instruction *, z3::expr>>
      getSuccsOfWithCond(const llvm::Instruction *stmt) override;
  bool isPPBranchTarget(const llvm::Instruction *stmt,
                        const llvm::Instruction *succ) override;
  bool isPPBranchTarget(const llvm::Instruction *stmt,
                        const llvm::Instruction *succ,
                        z3::expr &condition) override;
  z3::expr getTrueCondition() override;
};
} // namespace psr