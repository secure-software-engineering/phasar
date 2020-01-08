#include <deque>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstrTypes.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalCFG.h>

namespace psr {
LLVMBasedVariationalCFG::LLVMBasedVariationalCFG() : ctx(new z3::context()) {}

z3::expr
LLVMBasedVariationalCFG::createBinOp(const llvm::BinaryOperator *val) const {
  // TODO add, sub, mul, ...
  return getTrueCondition();
}
z3::expr LLVMBasedVariationalCFG::createVariableOrGlobal(
    const llvm::LoadInst *val) const {
  // TODO implement
  return getTrueCondition();
}
z3::expr
LLVMBasedVariationalCFG::createGEP(const llvm::GetElementPtrInst *val) const {
  // TODO implement
  return getTrueCondition();
}

z3::expr
LLVMBasedVariationalCFG::createExpression(const llvm::Value *val) const {
  if (auto load = llvm::dyn_cast<llvm::LoadInst>(val)) {
    return createVariableOrGlobal(load);
  } else if (auto cmp = llvm::dyn_cast<llvm::CmpInst>(val)) {
    return inferCondition(cmp);
  } else if (auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(val)) {
    return createGEP(gep);
  } else if (auto binop = llvm::dyn_cast<llvm::BinaryOperator>(val)) {
    return createBinOp(binop);
  } else if (auto fneg = llvm::dyn_cast<llvm::UnaryOperator>(val);
             fneg && fneg->getOpcode() == llvm::UnaryOperator::UnaryOps::FNeg) {
    return -createExpression(fneg->getOperand(0));
  }

  assert(false && "Unknown expression");
  return getTrueCondition();
}
z3::expr
LLVMBasedVariationalCFG::inferCondition(const llvm::CmpInst *cmp) const {
  auto lhs = cmp->getOperand(0);
  auto rhs = cmp->getOperand(1);

  auto xLhs = createExpression(lhs);
  auto xRhs = createExpression(rhs);

  switch (cmp->getOpcode()) {
  case llvm::CmpInst::ICMP_EQ:
  case llvm::CmpInst::FCMP_OEQ:
  case llvm::CmpInst::FCMP_UEQ:
    return xLhs == xRhs;
  case llvm::CmpInst::ICMP_NE:
  case llvm::CmpInst::FCMP_ONE:
  case llvm::CmpInst::FCMP_UNE:
    return xLhs != xRhs;
  case llvm::CmpInst::ICMP_SGE:
  case llvm::CmpInst::ICMP_UGE:
  case llvm::CmpInst::FCMP_OGE:
  case llvm::CmpInst::FCMP_UGE:
    return xLhs >= xRhs;
  case llvm::CmpInst::ICMP_SGT:
  case llvm::CmpInst::ICMP_UGT:
  case llvm::CmpInst::FCMP_OGT:
  case llvm::CmpInst::FCMP_UGT:
    return xLhs > xRhs;
  case llvm::CmpInst::ICMP_SLE:
  case llvm::CmpInst::ICMP_ULE:
  case llvm::CmpInst::FCMP_OLE:
  case llvm::CmpInst::FCMP_ULE:
    return xLhs <= xRhs;
  case llvm::CmpInst::ICMP_SLT:
  case llvm::CmpInst::ICMP_ULT:
  case llvm::CmpInst::FCMP_OLT:
  case llvm::CmpInst::FCMP_ULT:
    return xLhs < xRhs;
  case llvm::CmpInst::FCMP_FALSE:
    return ctx->bool_val(false);
  case llvm::CmpInst::FCMP_TRUE:
    return ctx->bool_val(true);
  case llvm::CmpInst::FCMP_UNO: // unordered (either nans)
  case llvm::CmpInst::FCMP_ORD: // ordered (no nans)
  default:                      // will not happen
    assert(false && "Invalid cmp instruction");
    return getTrueCondition();
  }

  return getTrueCondition();
}
bool LLVMBasedVariationalCFG::isPPBranchNode(const llvm::BranchInst *br) const {
  if (!br->isConditional())
    return false;
  // cond will most likely be an 'icmp ne i32 ..., 0'
  // it cannot be some logical con/disjunction, since this is modelled as
  // chained branches
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
                                             z3::expr &cond) const {
  if (!br->isConditional()) {
    cond = getTrueCondition();
    return false;
  }
  // cond will most likely be an 'icmp ne i32 ..., 0'
  // it cannot be some logical con/disjunction, since this is modelled as
  // chained branches
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
              cond = ctx->bool_val(true);
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
z3::expr LLVMBasedVariationalCFG::getTrueCondition() const {
  return ctx->bool_val(true);
}
bool LLVMBasedVariationalCFG::isPPBranchTarget(
    const llvm::Instruction *stmt, const llvm::Instruction *succ) const {
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
                                               z3::expr &condition) const {
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