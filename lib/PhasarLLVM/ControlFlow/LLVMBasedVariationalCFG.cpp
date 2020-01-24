/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalCFG.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace psr;

namespace psr {

z3::expr
LLVMBasedVariationalCFG::createBinOp(const llvm::BinaryOperator *val) const {
  auto lhs = val->getOperand(0);
  auto rhs = val->getOperand(1);

  auto xLhs = createExpression(lhs);
  auto xRhs = createExpression(rhs);

  switch (val->getOpcode()) {
  case llvm::BinaryOperator::BinaryOps::Add:
  case llvm::BinaryOperator::BinaryOps::FAdd:
    return xLhs + xRhs;
  case llvm::BinaryOperator::BinaryOps::And:
    return xLhs & xRhs;
  case llvm::BinaryOperator::BinaryOps::AShr:
    return z3::ashr(xLhs, xRhs);
  case llvm::BinaryOperator::BinaryOps::FDiv:
    return xLhs / xRhs;
  case llvm::BinaryOperator::BinaryOps::Mul:
  case llvm::BinaryOperator::BinaryOps::FMul:
    return xLhs * xRhs;
  case llvm::BinaryOperator::BinaryOps::FRem:
  case llvm::BinaryOperator::BinaryOps::SRem:
    return xLhs % xRhs;
  case llvm::BinaryOperator::BinaryOps::URem:
    return z3::urem(xLhs, xRhs);
  case llvm::BinaryOperator::BinaryOps::FSub:
    return xLhs - xRhs;
  case llvm::BinaryOperator::BinaryOps::LShr:
    return z3::lshr(xLhs, xRhs);
  case llvm::BinaryOperator::BinaryOps::Or:
    return xLhs | xRhs;
  case llvm::BinaryOperator::BinaryOps::Shl:
    return z3::shl(xLhs, xRhs);
  case llvm::BinaryOperator::BinaryOps::Sub:
  case llvm::BinaryOperator::BinaryOps::SDiv:
    // return (xLhs - xLhs % xRhs) / xRhs;
    return xLhs / xRhs;
  case llvm::BinaryOperator::BinaryOps::UDiv:
    return z3::udiv(xLhs, xRhs);
  case llvm::BinaryOperator::BinaryOps::Xor:
    return xLhs ^ xRhs;
  default:
    assert(false && "Invalid binary operator");
    return getTrueCondition();
  }
}

bool LLVMBasedVariationalCFG::isPPVariable(const llvm::GlobalVariable *glob,
                                           std::string &_name) const {
  auto name = glob->getName();
  if (!name.startswith("_")) {
    // std::cerr << "Not starts with _" << std::endl;
    return false;
  }
  auto config = name.find("_CONFIG_");
  const auto config_len = llvm::StringRef("_CONFIG_").size();
  if (config == llvm::StringRef::npos || config == 0 ||
      name.size() == config_len) {
    // std::cerr << "ERROR: config: " << config << "; size: " << name.size()
    //          << " vs " << config_len << std::endl;
    return false;
  }

  // if (name.endswith("_defined")) {
  // const auto defined_len = llvm::StringRef("_defined").size();
  // name = name.substr(0, name.size() - defined_len);
  //}

  _name = name.substr(config + config_len);

  return true;
}

z3::expr LLVMBasedVariationalCFG::createVariableOrGlobal(
    const llvm::LoadInst *val) const {
  auto pointerOp = val->getPointerOperand();
  if (auto glob = llvm::dyn_cast<llvm::GlobalVariable>(pointerOp)) {
    if (glob->isConstant() && glob->hasInitializer()) {
      return createConstant(glob->getInitializer());
    } else {
      std::string name;
      if (isPPVariable(glob, name)) {
        auto it = PPVariables.find(name);
        if (it != PPVariables.end()) {
          // std::cout << "Variable: " << it->second << std::endl;
          return it->second;
        }
        // constexpr auto defined_len = sizeof("_defined") - 1;
        auto ret = getTrueCondition();
        auto defined_pos = name.find_last_of("_defined");

        if (defined_pos == name.size() - 1) {
          ret = CTX.bool_const(name.data());
        } else {

          ret = CTX.int_const(name.data());
        }
        // move name here to preserve the allocated string, which is stored
        // untracked in ret
        PPVariables.insert({std::move(name), ret});
        // std::cout << "+Variable: " << ret << std::endl;
        return ret;
      } /*else {
        std::cerr << "ERROR: " << llvmIRToString(val) << " is no pp variable"
                  << std::endl;
      }*/
    }
  }
  // std::cerr << "Invalid preprocessor variable: " << llvmIRToString(val)
  //          << std::endl;
  assert(false && "Invalid preprocessor variable");
  return getTrueCondition();
}

z3::expr
LLVMBasedVariationalCFG::createConstant(const llvm::Constant *val) const {
  if (auto cnstInt = llvm::dyn_cast<llvm::ConstantInt>(val)) {
    if (cnstInt->getValue().getBitWidth() == 1)
      return CTX.bool_val(cnstInt->getLimitedValue());
    else
      return CTX.int_val(cnstInt->getSExtValue());
  } else if (auto cnstFP = llvm::dyn_cast<llvm::ConstantFP>(val)) {
    return CTX.fpa_val(cnstFP->getValueAPF().convertToDouble());
  } else {
    assert(false && "Invalid constant value");
    return getTrueCondition();
  }
}

z3::expr
LLVMBasedVariationalCFG::createExpression(const llvm::Value *val) const {
  if (auto load = llvm::dyn_cast<llvm::LoadInst>(val)) {
    return createVariableOrGlobal(load);
  } else if (auto cmp = llvm::dyn_cast<llvm::CmpInst>(val)) {
    return inferCondition(cmp);
  } else if (auto binop = llvm::dyn_cast<llvm::BinaryOperator>(val)) {
    return createBinOp(binop);
  } else if (auto fneg = llvm::dyn_cast<llvm::UnaryOperator>(val);
             fneg && fneg->getOpcode() == llvm::UnaryOperator::UnaryOps::FNeg) {
    return -createExpression(fneg->getOperand(0));
  } else if (auto cnst = llvm::dyn_cast<llvm::Constant>(val)) {
    return createConstant(cnst);
  }

  assert(false && "Unknown expression");
  return getTrueCondition();
}

z3::expr LLVMBasedVariationalCFG::compareBoolAndInt(z3::expr xBool,
                                                    z3::expr xInt,
                                                    bool forEquality) const {
  std::cout << "Compare bool and int: " << xBool << (forEquality ? "==" : "!=")
            << xInt << std::endl;
  int64_t intVal;
  if (xInt.is_numeral_i64(intVal)) {
    if (forEquality)
      return intVal ? xBool : !xBool;
    else
      return intVal ? !xBool : xBool;
  }
  auto ret = (xInt != CTX.int_val(0)) == xBool;
  if (!forEquality)
    ret = !ret;
  return ret;
}

z3::expr
LLVMBasedVariationalCFG::inferCondition(const llvm::CmpInst *cmp) const {
  auto lhs = cmp->getOperand(0);
  auto rhs = cmp->getOperand(1);

  auto xLhs = createExpression(lhs);
  auto xRhs = createExpression(rhs);
  int64_t rhsVal;
  if (cmp->isEquality()) {
    if (xLhs.is_bool() && xRhs.is_int())
      return compareBoolAndInt(xLhs, xRhs,
                               cmp->getPredicate() == llvm::CmpInst::ICMP_EQ);
    else if (xLhs.is_int() && xRhs.is_bool())
      return compareBoolAndInt(xRhs, xLhs,
                               cmp->getPredicate() == llvm::CmpInst::ICMP_EQ);
  }
  // std::cout << "inferCondition: xLhs = " << xLhs << "; xRhs = " << xRhs
  //          << std::endl;

  switch (cmp->getPredicate()) {
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
    return CTX.bool_val(false);
  case llvm::CmpInst::FCMP_TRUE:
    return CTX.bool_val(true);
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
          std::string name;
          if (isPPVariable(glob, name))
            return true;
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
    std::string name;
    for (auto &use : condUser->operands()) {
      if (auto load = llvm::dyn_cast<llvm::LoadInst>(use.get())) {
        if (auto glob = llvm::dyn_cast<llvm::GlobalVariable>(
                load->getPointerOperand())) {

          if (isPPVariable(glob, name)) {
            if (auto icmp = llvm::dyn_cast<llvm::CmpInst>(lcond)) {
              cond = inferCondition(icmp);
            } else {
              // TODO was hier?
              cond = CTX.bool_val(true);
            }
            return true;
          } /*else {
            std::cout << glob->getName().str() << " is no PP variable"
                      << std::endl;
          }*/
        }
      }
    }
    // std::cerr << "Fall through" << std::endl;
  } else
    std::cerr << "No user" << std::endl;
  cond = getTrueCondition();
  return false;
}

std::vector<std::pair<const llvm::Instruction *, z3::expr>>
LLVMBasedVariationalCFG::getSuccsOfWithCond(
    const llvm::Instruction *stmt) const {
  std::vector<std::pair<const llvm::Instruction *, z3::expr>> Successors;
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
  return CTX.bool_val(true);
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

z3::context &LLVMBasedVariationalCFG::getContext() const { return CTX; }

} // namespace psr
