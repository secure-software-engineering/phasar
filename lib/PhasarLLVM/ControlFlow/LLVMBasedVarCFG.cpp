/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <z3++.h>

#include "llvm/IR/CFG.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarCFG.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace psr;

namespace psr {
#if 0
z3::expr LLVMBasedVarCFG::createBinOp(const llvm::BinaryOperator *val) const {
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
    llvm::report_fatal_error("Invalid binary operator");
  }
}

bool LLVMBasedVarCFG::isPPVariable(const llvm::GlobalVariable *glob,
                                   std::string &_name) const {
  auto name = glob->getName();
  if (!name.startswith("__static_condition")) {
    return false;
  }

  constexpr auto config_len = sizeof("__static_condition") - 1;
  if (name.size() == config_len) {
    return false;
  }

  // TODO: extract name from __static_condition_renaming call in
  // __static_initializer
  _name = name.str();

  // std::cout << "Found static condition: " << name.str() << std::endl;

  return true;
}

// don't pass by reference, as we need to take ownership of the Name
z3::expr LLVMBasedVarCFG::createBoolConst(std::string Name) const {
  auto ret = CTX.bool_const(Name.data());
  // move name here to preserve the allocated string which is stored
  // untracked in ret
  PPVariables.insert({std::move(Name), ret});
  return ret;
}
// don't pass by reference, as we need to take ownership of the Name
z3::expr LLVMBasedVarCFG::createIntConst(std::string Name) const {
  auto ret = CTX.int_const(Name.data());
  // move name here to preserve the allocated string which is stored
  // untracked in ret
  PPVariables.insert({std::move(Name), ret});
  return ret;
}

z3::expr
LLVMBasedVarCFG::createVariableOrGlobal(const llvm::LoadInst *val) const {
  auto pointerOp = val->getPointerOperand();
  if (auto glob = llvm::dyn_cast<llvm::GlobalVariable>(pointerOp)) {
    if (glob->isConstant() && glob->hasInitializer()) {
      return createConstant(glob->getInitializer());
    } else {
      std::string name;
      if (isPPVariable(glob, name)) {
        auto it = PPVariables.find(name);
        if (it != PPVariables.end()) {
          return it->second;
        }

        // auto ret = getTrueConstraint();

        // TODO: the naming has changed in XTC; especially, we no longer have
        // the _defined suffix.
        auto defined_pos = name.find_last_of("_defined");

        if (defined_pos == name.size() - 1) {
          // ret = CTX.bool_const(name.data());
          return createBoolConst(std::move(name));
        } else {

          // ret = CTX.int_const(name.data());
          return createIntConst(std::move(name));
        }
      }
    }
  }
  llvm::report_fatal_error("Invalid preprocessor variable");
}

z3::expr LLVMBasedVarCFG::createConstant(const llvm::Constant *val) const {
  if (auto cnstInt = llvm::dyn_cast<llvm::ConstantInt>(val)) {
    if (cnstInt->getValue().getBitWidth() == 1)
      return CTX.bool_val(cnstInt->getLimitedValue());
    else
      return CTX.int_val(cnstInt->getSExtValue());
  } else if (auto cnstFP = llvm::dyn_cast<llvm::ConstantFP>(val)) {
    return CTX.fpa_val(cnstFP->getValueAPF().convertToDouble());
  } else {
    llvm::report_fatal_error("Invalid constant value");
  }
}

z3::expr LLVMBasedVarCFG::createExpression(const llvm::Value *val) const {
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

  llvm::report_fatal_error("Unknown expression");
}

z3::expr LLVMBasedVarCFG::compareBoolAndInt(z3::expr xBool, z3::expr xInt,
                                            bool forEquality) const {
  // std::cout << "Compare bool and int: " << xBool << (forEquality ? "==" :
  // "!=")
  // << xInt << std::endl;
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

z3::expr LLVMBasedVarCFG::inferCondition(const llvm::CmpInst *cmp) const {
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
    llvm::report_fatal_error("Invalid cmp instruction");
  }

  return getTrueConstraint();
}
#endif

LLVMBasedVarCFG::LLVMBasedVarCFG(
    const ProjectIRDB &IRDB, const stringstringmap_t *StaticBackwardRenaming)
    : staticBackwardRenaming(StaticBackwardRenaming) {
  // void __static_condition_renaming(globName, smt2lib_solver)
  auto staticRenamingFn = IRDB.getFunction("__static_condition_renaming");
  // We need to check if staticRenamingFn is null. In case no static
  // preprocessor conditionals are beeing used, this function does not exist and
  // thus the ProjectIRDB returns a nullptr.
  if (staticRenamingFn) {
    for (auto use : staticRenamingFn->users()) {
      if (auto call = llvm::dyn_cast<llvm::CallBase>(use);
          call && call->getCalledFunction() == staticRenamingFn) {
        auto globName =
            extractConstantStringFromValue(call->getArgOperand(0)).value();
        auto smt2lib_solver =
            extractConstantStringFromValue(call->getArgOperand(1)).value();

        z3::solver Solver(CTX);
        Solver.from_string(smt2lib_solver.data());
        auto assertions = Solver.assertions();
        assert(assertions.size() == 1);
        AvailablePPConditions.insert({globName, assertions[0]});
      }
    }
  }
}

std::optional<z3::expr> LLVMBasedVarCFG::getConditionIfIsPPVariable(
    const llvm::GlobalVariable *G) const {
  auto name = G->getName();
  constexpr char STATIC_RENAMING[] = "__static_condition_renaming";
  if (name.size() <= sizeof(STATIC_RENAMING) ||
      !name.startswith(STATIC_RENAMING)) {
    return std::nullopt;
  }

  if (auto it = AvailablePPConditions.find(name);
      it != AvailablePPConditions.end()) {
    return it->second;
  }

  return std::nullopt;
}

bool LLVMBasedVarCFG::isPPBranchNode(const llvm::BranchInst *br) const {
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
          // std::string name;
          // if (isPPVariable(glob, name))
          if (getConditionIfIsPPVariable(glob).has_value())
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

bool LLVMBasedVarCFG::isPPBranchNode(const llvm::BranchInst *br,
                                     z3::expr &cond) const {
  if (!br->isConditional()) {
    cond = getTrueConstraint();
    return false;
  }
  // cond will most likely be an 'icmp ne i32 ..., 0'
  // it cannot be some logical con/disjunction, since this is modelled as
  // chained branches
  auto lcond = br->getCondition();

#if 0
  if (auto condUser = llvm::dyn_cast<llvm::User>(lcond)) {
    // check for 'load i32, i32* @_Z...CONFIG_...'
    // TODO: Is normal iteration sufficient, or do we need recursion here?
    // std::string name;
    for (auto &use : condUser->operands()) {
      if (auto load = llvm::dyn_cast<llvm::LoadInst>(use.get())) {
        if (auto glob = llvm::dyn_cast<llvm::GlobalVariable>(
                load->getPointerOperand())) {

          // if (isPPVariable(glob, name)) {
          if (auto solverSpec = getConditionIfIsPPVariable(glob);
              solverSpec.has_value()) {
            if (auto icmp = llvm::dyn_cast<llvm::CmpInst>(lcond)) {
              cond = inferCondition(icmp);
            } else if (auto trnc = llvm::dyn_cast<llvm::TruncInst>(lcond);
                       trnc && trnc->getType()->isIntegerTy(1)) {
              cond = createBoolConst(std::move(name));
            } else {
              // TODO was hier?
              std::cerr << "Fallback to true" << std::endl;
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
#endif
  cond = getTrueConstraint();
  return false;
}

std::vector<std::pair<const llvm::Instruction *, z3::expr>>
LLVMBasedVarCFG::getSuccsOfWithPPConstraints(
    const llvm::Instruction *Stmt) const {
  std::vector<std::pair<const llvm::Instruction *, z3::expr>> Successors;
  for (auto Succ : llvm::successors(Stmt)) {
    Successors.emplace_back(&Succ->front(),
                            getPPConstraintOrTrue(Stmt, &Succ->front()));
  }
  return Successors;
}

z3::expr LLVMBasedVarCFG::getTrueConstraint() const {
  return CTX.bool_val(true);
}

bool LLVMBasedVarCFG::isPPBranchTarget(const llvm::Instruction *Stmt,
                                       const llvm::Instruction *Succ) const {
  if (auto *T = llvm::dyn_cast<llvm::BranchInst>(Stmt)) {
    if (!isPPBranchNode(T))
      return false;
    for (auto Successor : T->successors()) {
      if (&Successor->front() == Succ) {
        return true;
      }
    }
  }
  return false;
}

z3::expr
LLVMBasedVarCFG::getPPConstraintOrTrue(const llvm::Instruction *Stmt,
                                       const llvm::Instruction *Succ) const {
  z3::expr Constraint = getTrueConstraint();
  if (auto B = llvm::dyn_cast<llvm::BranchInst>(Stmt);
      B && B->isConditional()) {
    if (isPPBranchNode(B, Constraint)) {
      // num successors == 2
      if (Succ == &B->getSuccessor(0)->front()) { // then-branch
        return Constraint;
      } else if (Succ == &B->getSuccessor(1)->front()) { // else-branch
        return !Constraint;
      }
    }
  }
  return Constraint;
}

z3::context &LLVMBasedVarCFG::getContext() const { return CTX; }

std::string
LLVMBasedVarCFG::getDemangledFunctionName(const llvm::Function *Fun) const {
  auto fnName = this->LLVMBasedCFG::getDemangledFunctionName(Fun);
  if (!staticBackwardRenaming)
    return fnName;

  if (auto it = staticBackwardRenaming->find(fnName);
      it != staticBackwardRenaming->end())
    return it->getValue();

  return fnName;
}

} // namespace psr
