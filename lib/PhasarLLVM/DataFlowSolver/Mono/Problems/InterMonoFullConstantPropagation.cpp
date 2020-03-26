/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann and others
 *****************************************************************************/

#include <algorithm>
#include <llvm/Support/Casting.h>
#include <ostream>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoFullConstantPropagation.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/BitVectorSet.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <unordered_map>

using namespace std;
using namespace psr;

namespace psr {

InterMonoFullConstantPropagation::InterMonoFullConstantPropagation(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : InterMonoProblem<InterMonoFullConstantPropagation::n_t,
                       InterMonoFullConstantPropagation::d_t,
                       InterMonoFullConstantPropagation::f_t,
                       InterMonoFullConstantPropagation::t_t,
                       InterMonoFullConstantPropagation::v_t,
                       InterMonoFullConstantPropagation::i_t>(IRDB, TH, ICF, PT,
                                                              EntryPoints) {}

bool InterMonoFullConstantPropagation::bitVectorHasInstr(
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &set,
    InterMonoFullConstantPropagation::v_t instr) {
  for (auto e : set.getAsSet()) {
    if (e.first == instr) {
      return true;
    }
  }
  return false;
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::join(
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Rhs) {
  return merge(update(Lhs, Rhs), Rhs);
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::merge(
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Rhs) {
  BitVectorSet<InterMonoFullConstantPropagation::d_t> Out = Lhs;
  for (auto elem : Rhs.getAsSet()) {
    if (!bitVectorHasInstr(Lhs, elem.first)) {
      Out.insert(elem);
    }
  }
  return Out;
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::update(
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Rhs) {
  BitVectorSet<InterMonoFullConstantPropagation::d_t> Out = Lhs;
  for (auto elem : Rhs.getAsSet()) {
    if (bitVectorHasInstr(Lhs, elem.first)) {
      for (auto elen : Out.getAsSet()) {
        if (elem.first == elen.first &&
            (std::holds_alternative<Top>(elen.second) ||
             std::holds_alternative<Bottom>(elem.second))) {
          Out.erase(elen);
          Out.insert(elem);
        } else if (elem.first == elen.first &&
                   (std::holds_alternative<plain_d_t>(elem.second) &&
                    std::holds_alternative<plain_d_t>(elen.second) &&
                    *std::get_if<plain_d_t>(&elem.second) !=
                        *std::get_if<plain_d_t>(&elen.second))) {

          Out.erase(elen);
          Out.insert({elem.first, Bottom{}});

        }
      }
    }
  }
  return Out;
}

bool InterMonoFullConstantPropagation::sqSubSetEqual(
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Rhs) {

  return Rhs.includes(Lhs);
}

bool InterMonoFullConstantPropagation::equal_to(
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Rhs) {
  return Rhs == Lhs;
}

std::unordered_map<InterMonoFullConstantPropagation::n_t,
                   BitVectorSet<InterMonoFullConstantPropagation::d_t>>
InterMonoFullConstantPropagation::initialSeeds() {
  std::unordered_map<InterMonoFullConstantPropagation::n_t,
                     BitVectorSet<InterMonoFullConstantPropagation::d_t>>
      Seeds;
  for (auto &EntryPoint : EntryPoints) {
    if (auto Fun = IRDB->getFunctionDefinition(EntryPoint)) {
      auto Is = ICF->getStartPointsOf(Fun);
      for (auto I : Is) {
        Seeds[I] = {};
      }
    }
  }
  return Seeds;
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::normalFlow(
    InterMonoFullConstantPropagation::n_t S,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &In) {
  auto Out = In;

  // check Alloca instructions
  if (auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(S)) {
    if (Alloc->getAllocatedType()->isIntegerTy()) {
      Out.insert({Alloc, Top{}});
    }
  }

  // check store instructions
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(S)) {
    auto ValueOp = Store->getValueOperand();
    // Case I: Integer literal
    if (auto val = llvm::dyn_cast<llvm::ConstantInt>(ValueOp)) {
      for (auto elem : In.getAsSet()) {
        if (elem.first == Store->getPointerOperand()) {
          if (std::holds_alternative<Bottom>(elem.second)) {
            break;
          }

          Out.erase(elem);
          Out.insert({Store->getPointerOperand(), val->getSExtValue()});
          return Out;
        }
      }
    }
    // Case II: Storing an integer typed value
    if (ValueOp->getType()->isIntegerTy()) {
      LatticeDomain<InterMonoFullConstantPropagation::plain_d_t> latticeVal = Top{};
      for (auto elem : In.getAsSet()) {
        if (elem.first == ValueOp) {
          latticeVal = elem.second;
          break;
        }
      }
      if (!std::holds_alternative<Top>(latticeVal)) {
        for (auto elem : In.getAsSet()) {
          if (elem.first == Store->getPointerOperand()) {
            if (std::holds_alternative<Bottom>(elem.second)) {
              break;
            }

            Out.erase(elem);
            Out.insert({Store->getPointerOperand(), latticeVal});
            return Out;
          }
        }
      }
    }
  }

  // check load instructions
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(S)) {
    LatticeDomain<InterMonoFullConstantPropagation::plain_d_t> latticeVal;
    for (auto elem : In.getAsSet()) {
      if (elem.first == Load->getPointerOperand()) {
        latticeVal = elem.second;
        break;
      }
    }
    Out.insert({Load, latticeVal});

    return Out;
  }

  // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
  if (auto op = llvm::dyn_cast<llvm::BinaryOperator>(S)) {
    auto lop = S->getOperand(0);
    auto rop = S->getOperand(1);
    LatticeDomain<InterMonoFullConstantPropagation::plain_d_t> lval;
    LatticeDomain<InterMonoFullConstantPropagation::plain_d_t> rval;

    // get first operand value
    if (auto val = llvm::dyn_cast<llvm::ConstantInt>(lop)) {
      lval = val->getSExtValue();
    } else {
      for (auto elem : In.getAsSet()) {
        if (elem.first == lop) {
          lval = elem.second;
          break;
        }
      }
    }

    // get second operand value
    if (auto val = llvm::dyn_cast<llvm::ConstantInt>(rop)) {
      rval = val->getSExtValue();
    } else {
      for (auto elem : In.getAsSet()) {
        if (elem.first == rop) {
          rval = elem.second;
          break;
        }
      }
    }

    // handle Top or Bottom as a operand
    if (std::holds_alternative<Top>(lval) ||
        std::holds_alternative<Top>(rval) ||
        std::holds_alternative<Bottom>(lval) ||
        std::holds_alternative<Bottom>(rval)) {

      Out.insert({op, Bottom{}});
      return Out;

    } else {
      Out.insert({S, executeBinOperation(op->getOpcode(),
                                         *std::get_if<plain_d_t>(&lval),
                                         *std::get_if<plain_d_t>(&rval))});
      return Out;
    }
  }

  return Out;
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::callFlow(
    InterMonoFullConstantPropagation::n_t CallSite,
    InterMonoFullConstantPropagation::f_t Callee,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &In) {
  /*  auto Out = In;
   if (auto A = llvm::dyn_cast<llvm::Argument>(destNode)) {
     llvm::ImmutableCallSite CS(CallSite);
     auto actual = CS.getArgOperand(getFunctionArgumentNr(A));
     if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(actual)) {
       auto IntConst = CI->getSExtValue();
       Out.insert({actual,IntConst});
       return Out;
     }
   } */
  return In;
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::returnFlow(
    InterMonoFullConstantPropagation::n_t CallSite,
    InterMonoFullConstantPropagation::f_t Callee,
    InterMonoFullConstantPropagation::n_t ExitStmt,
    InterMonoFullConstantPropagation::n_t RetSite,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &In) {
  auto Out = In;
  if (CallSite->getType()->isIntegerTy()) {
    auto Return = llvm::dyn_cast<llvm::ReturnInst>(ExitStmt);
    auto ReturnValue = Return->getReturnValue();

    // Kill everything that is not returned
    Out.clear();

    // Return value is integer literal
    if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(ReturnValue)) {
      Out.insert({CallSite, CI->getSExtValue()});
      return Out;
    }
    // handle Global Variables
    // TODO:handle globals
  }
  Out.clear();
  return Out;
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::callToRetFlow(
    InterMonoFullConstantPropagation::n_t CallSite,
    InterMonoFullConstantPropagation::n_t RetSite,
    std::set<InterMonoFullConstantPropagation::f_t> Callees,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &In) {
  // TODO implement
  return In;
}

LatticeDomain<InterMonoFullConstantPropagation::plain_d_t>
InterMonoFullConstantPropagation::executeBinOperation(
    const unsigned op, InterMonoFullConstantPropagation::plain_d_t lop,
    InterMonoFullConstantPropagation::plain_d_t rop) {
  // default initialize with BOTTOM (all information)
  LatticeDomain<InterMonoFullConstantPropagation::plain_d_t> res = Bottom{};
  switch (op) {
  case llvm::Instruction::Add:
    res = lop + rop;
    break;

  case llvm::Instruction::Sub:
    res = lop - rop;
    break;

  case llvm::Instruction::Mul:
    res = lop * rop;
    break;

  case llvm::Instruction::UDiv:
  case llvm::Instruction::SDiv:
    res = lop / rop;
    break;

  case llvm::Instruction::URem:
  case llvm::Instruction::SRem:
    res = lop % rop;
    break;

  default:
    break;
  }
  return res;
}

void InterMonoFullConstantPropagation::printNode(
    std::ostream &os, InterMonoFullConstantPropagation::n_t n) const {
  os << llvmIRToString(n);
}

void InterMonoFullConstantPropagation::printDataFlowFact(
    std::ostream &os, InterMonoFullConstantPropagation::d_t d) const {
  os << "< " + llvmIRToString(d.first) << ", ";
  if (std::holds_alternative<Top>(d.second)) {
    os << std::get<Top>(d.second);
  }
  if (std::holds_alternative<Bottom>(d.second)) {
    os << std::get<Bottom>(d.second);
  }
  if (std::holds_alternative<InterMonoFullConstantPropagation::plain_d_t>(
          d.second)) {
    os << std::get<InterMonoFullConstantPropagation::plain_d_t>(d.second);
  }
  os << " >\n";
}

void InterMonoFullConstantPropagation::printFunction(
    std::ostream &os, InterMonoFullConstantPropagation::f_t f) const {
  os << f->getName().str();
}

} // namespace psr
