#include <deque>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstrTypes.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalCFG.h>

namespace psr {
z3::expr LLVMBasedVariationalCFG::inferCondition(const llvm::CmpInst *cmp) {
  // TODO implement
  return getTrueCondition();
}
bool LLVMBasedVariationalCFG::isPPBranchNode(const llvm::BranchInst *br) {
  if (!br->isConditional())
    return false;
  // cond will most likely be an 'icmp ne i32 ..., 0'
  auto cond = br->getCondition();
  if (auto condUser = llvm::dyn_cast<llvm::User>(cond)) {
    // check for 'load i32, i32* @_Z...CONFIG_...'
    // TODO: Is normal iteration sufficient, or do we need recursion here?
    for (auto &use : condUser->operands()) {
      if (auto load = llvm::dyn_cast<llvm::LoadInst>(use.get())) {
        if (auto glob = llvm::dyn_cast<llvm::GlobalVariable>(
                load->getPointerOperand())) {
          auto name = glob->getName();
          if (name.contains("_CONFIG_") &&
              name.size() > llvm::StringRef("_CONFIG_").size()) {
            return true;
          }
        }
      }
    }
  } else {
    // What if cond is no user?
    return false;
  }
  return false;
}
bool LLVMBasedVariationalCFG::isPPBranchNode(const llvm::BranchInst *br,
                                             z3::expr &cond) {
  if (!br->isConditional()) {
    cond = getTrueCondition();
    return false;
  }
  // cond will most likely be an 'icmp ne i32 ..., 0'
  auto lcond = br->getCondition();
  if (auto condUser = llvm::dyn_cast<llvm::User>(lcond)) {
    // check for 'load i32, i32* @_Z...CONFIG_...'
    // TODO: Is normal iteration sufficient, or do we need recursion here?
    for (auto &use : condUser->operands()) {
      if (auto load = llvm::dyn_cast<llvm::LoadInst>(use.get())) {
        if (auto glob = llvm::dyn_cast<llvm::GlobalVariable>(
                load->getPointerOperand())) {
          auto name = glob->getName();
          if (name.contains("_CONFIG_") &&
              name.size() > llvm::StringRef("_CONFIG_").size()) {
            if (auto icmp = llvm::dyn_cast<llvm::CmpInst>(lcond)) {
              cond = inferCondition(icmp);
            } else {
              // TODO was hier?
              cond = ctx.bool_val(true);
            }
            return true;
          }
        }
      }
    }
  }
  cond = getTrueCondition();
  return false;
}
std::vector<std::tuple<const llvm::Instruction *, z3::expr>>
LLVMBasedVariationalCFG::getSuccsOfWithCond(const llvm::Instruction *stmt) {

  std::vector<std::tuple<const llvm::Instruction *, z3::expr>> Successors;
  if (stmt->getNextNode()) {
    Successors.emplace_back(stmt->getNextNode(), getTrueCondition());
  }
  if (stmt->isTerminator()) {
    for (unsigned i = 0; i < stmt->getNumSuccessors(); ++i) {
      auto succ = &*stmt->getSuccessor(i)->begin();
      z3::expr cond = getTrueCondition();
      if (isPPBranchTarget(stmt, succ, cond))
        Successors.emplace_back(succ, cond);
      else
        Successors.emplace_back(succ, getTrueCondition());
    }
  }
  return Successors;
}
z3::expr LLVMBasedVariationalCFG::getTrueCondition() {
  return ctx.bool_val(true);
}
bool LLVMBasedVariationalCFG::isPPBranchTarget(const llvm::Instruction *stmt,
                                               const llvm::Instruction *succ) {
  if (auto *T = llvm::dyn_cast<llvm::BranchInst>(stmt)) {
    if (!isPPBranchNode(T))
      return false;
    for (auto successor : T->successors()) {
      if (&*successor->begin() == succ) {
        return true;
      }
    }
  }
  return false;
}
bool LLVMBasedVariationalCFG::isPPBranchTarget(const llvm::Instruction *stmt,
                                               const llvm::Instruction *succ,
                                               z3::expr &condition) {
  if (auto br = llvm::dyn_cast<llvm::BranchInst>(stmt);
      br && br->isConditional()) {
    if (isPPBranchNode(br, condition)) {

      // num successors == 2
      if (succ == &*br->getSuccessor(0)->begin()) // then-branch
        return true;
      else if (succ == &*br->getSuccessor(1)->begin()) { // else-branch
        condition = !condition;
        return true;
      }
    }
  }
  return false;
}
} // namespace psr