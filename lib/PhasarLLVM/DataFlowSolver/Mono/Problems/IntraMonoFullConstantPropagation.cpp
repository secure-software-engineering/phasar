/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann and others
 *****************************************************************************/

#include <algorithm>
#include <ostream>
#include <utility>

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace std {
template <> struct hash<pair<const llvm::Value *, unsigned>> {
  size_t operator()(const pair<const llvm::Value *, unsigned> &P) const {
    std::hash<const llvm::Value *> HashPtr;
    std::hash<unsigned> HashUnsigned;
    size_t HP = HashPtr(P.first);
    size_t HU = HashUnsigned(P.second);
    return HP ^ (HU << 1);
  }
};
} // namespace std

using namespace psr;
namespace psr {

IntraMonoFullConstantPropagation::IntraMonoFullConstantPropagation(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedCFG *CF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IntraMonoProblem<IntraMonoFullConstantPropagation::n_t,
                       IntraMonoFullConstantPropagation::d_t,
                       IntraMonoFullConstantPropagation::f_t,
                       IntraMonoFullConstantPropagation::t_t,
                       IntraMonoFullConstantPropagation::v_t,
                       IntraMonoFullConstantPropagation::i_t>(
          IRDB, TH, CF, PT, std::move(EntryPoints)) {}

bool IntraMonoFullConstantPropagation::bitVectorHasInstr(
    const BitVectorSet<IntraMonoFullConstantPropagation::d_t> &set,
    IntraMonoFullConstantPropagation::v_t instr) {
  for (auto e : set) {
    if (e.first == instr) {
      return true;
    }
  }
  return false;
}

BitVectorSet<IntraMonoFullConstantPropagation::d_t>
IntraMonoFullConstantPropagation::merge(
    const BitVectorSet<IntraMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<IntraMonoFullConstantPropagation::d_t> &Rhs) {
  BitVectorSet<IntraMonoFullConstantPropagation::d_t> Out = Lhs;
  for (auto elem : Rhs) {
    if (!bitVectorHasInstr(Lhs, elem.first)) {
      Out.insert(elem);
    }
  }
  return Out;
}

BitVectorSet<IntraMonoFullConstantPropagation::d_t>
IntraMonoFullConstantPropagation::update(
    const BitVectorSet<IntraMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<IntraMonoFullConstantPropagation::d_t> &Rhs) {
  BitVectorSet<IntraMonoFullConstantPropagation::d_t> Out = Lhs;
  for (auto elem : Rhs) {
    if (bitVectorHasInstr(Lhs, elem.first)) {
      for (auto elen : Out) {
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

bool IntraMonoFullConstantPropagation::equal_to(
    const BitVectorSet<IntraMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<IntraMonoFullConstantPropagation::d_t> &Rhs) {
  return Rhs == Lhs;
}

std::unordered_map<IntraMonoFullConstantPropagation::n_t,
                   BitVectorSet<IntraMonoFullConstantPropagation::d_t>>
IntraMonoFullConstantPropagation::initialSeeds() {
  std::unordered_map<IntraMonoFullConstantPropagation::n_t,
                     BitVectorSet<IntraMonoFullConstantPropagation::d_t>>
      Seeds;
  for (auto &EntryPoint : EntryPoints) {
    if (auto Fun = IRDB->getFunctionDefinition(EntryPoint)) {
      auto Is = CF->getStartPointsOf(Fun);
      for (auto I : Is) {
        Seeds[I] = {};
      }
    }
  }
  return Seeds;
}

BitVectorSet<IntraMonoFullConstantPropagation::d_t>
IntraMonoFullConstantPropagation::normalFlow(
    IntraMonoFullConstantPropagation::n_t S,
    const BitVectorSet<IntraMonoFullConstantPropagation::d_t> &In) {
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
      for (auto elem : In) {
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
      LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> latticeVal =
          Top{};
      for (auto elem : In) {
        if (elem.first == ValueOp) {
          latticeVal = elem.second;
          break;
        }
      }
      if (!std::holds_alternative<Top>(latticeVal)) {
        for (auto elem : In) {
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
    LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> latticeVal;
    for (auto elem : In) {
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
    LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> lval;
    LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> rval;

    // get first operand value
    if (auto val = llvm::dyn_cast<llvm::ConstantInt>(lop)) {
      lval = val->getSExtValue();
    } else {
      for (auto elem : In) {
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
      for (auto elem : In) {
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

LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>
IntraMonoFullConstantPropagation::executeBinOperation(
    const unsigned op, IntraMonoFullConstantPropagation::plain_d_t lop,
    IntraMonoFullConstantPropagation::plain_d_t rop) {
  // default initialize with BOTTOM (all information)
  LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> res = Bottom{};
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

void IntraMonoFullConstantPropagation::printNode(
    std::ostream &os, IntraMonoFullConstantPropagation::n_t n) const {
  os << llvmIRToString(n);
}

void IntraMonoFullConstantPropagation::printDataFlowFact(
    std::ostream &os, IntraMonoFullConstantPropagation::d_t d) const {
  os << "< " + llvmIRToString(d.first) << ", ";
  if (std::holds_alternative<Top>(d.second)) {
    os << std::get<Top>(d.second);
  }
  if (std::holds_alternative<Bottom>(d.second)) {
    os << std::get<Bottom>(d.second);
  }
  if (std::holds_alternative<IntraMonoFullConstantPropagation::plain_d_t>(
          d.second)) {
    os << std::get<IntraMonoFullConstantPropagation::plain_d_t>(d.second);
  }
  os << " >\n";
}

void IntraMonoFullConstantPropagation::printFunction(
    std::ostream &os, IntraMonoFullConstantPropagation::f_t f) const {
  os << f->getName().str();
}

} // namespace psr
