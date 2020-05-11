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
                       IntraMonoFullConstantPropagation::i_t,
                       IntraMonoFullConstantPropagation::container_t>(
          IRDB, TH, CF, PT, std::move(EntryPoints)) {}

IntraMonoFullConstantPropagation::container_t
IntraMonoFullConstantPropagation::merge(
    const IntraMonoFullConstantPropagation::container_t &Lhs,
    const IntraMonoFullConstantPropagation::container_t &Rhs) {
  IntraMonoFullConstantPropagation::container_t Out;
  for (const auto &[Key, Value] : Lhs) {
    auto Search = Rhs.find(Key);
    if (Search != Rhs.end() && Value == Search->second) {
      Out.insert({Key, Value});
    }
  }
  return Out;
}

bool IntraMonoFullConstantPropagation::equal_to(
    const IntraMonoFullConstantPropagation::container_t &Lhs,
    const IntraMonoFullConstantPropagation::container_t &Rhs) {
  return Rhs == Lhs;
}

std::unordered_map<IntraMonoFullConstantPropagation::n_t,
                   IntraMonoFullConstantPropagation::container_t>
IntraMonoFullConstantPropagation::initialSeeds() {
  std::unordered_map<IntraMonoFullConstantPropagation::n_t,
                     IntraMonoFullConstantPropagation::container_t>
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

IntraMonoFullConstantPropagation::container_t
IntraMonoFullConstantPropagation::normalFlow(
    IntraMonoFullConstantPropagation::n_t S,
    const IntraMonoFullConstantPropagation::container_t &In) {
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
      for (const auto &[Variable, Value] : In) {
        if (Variable == Store->getPointerOperand()) {
          if (std::holds_alternative<Bottom>(Value)) {
            break;
          }

          Out.erase(Variable);
          Out.insert({Store->getPointerOperand(), val->getSExtValue()});
          return Out;
        }
      }
    }
    // Case II: Storing an integer typed value
    if (ValueOp->getType()->isIntegerTy()) {
      LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> latticeVal =
          Top{};
      for (const auto &[Variable, Value] : In) {
        if (Variable == ValueOp) {
          latticeVal = Value;
          break;
        }
      }
      if (!std::holds_alternative<Top>(latticeVal)) {
        for (const auto &[Variable, Value] : In) {
          if (Variable == Store->getPointerOperand()) {
            if (std::holds_alternative<Bottom>(Value)) {
              break;
            }

            Out.erase(Variable);
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
    for (const auto &[Variable, Value] : In) {
      if (Variable == Load->getPointerOperand()) {
        latticeVal = Value;
        break;
      }
    }
    Out.insert({Load, latticeVal});

    return Out;
  }

  // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
  if (auto Op = llvm::dyn_cast<llvm::BinaryOperator>(S)) {
    auto Lop = S->getOperand(0);
    auto Rop = S->getOperand(1);
    LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> lval;
    LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> rval;

    // get first operand value
    if (auto val = llvm::dyn_cast<llvm::ConstantInt>(Lop)) {
      lval = val->getSExtValue();
    } else {
      for (const auto &[Variable, Value] : In) {
        if (Variable == Lop) {
          lval = Value;
          break;
        }
      }
    }

    // get second operand value
    if (auto val = llvm::dyn_cast<llvm::ConstantInt>(Rop)) {
      rval = val->getSExtValue();
    } else {
      for (const auto &[Variable, Value] : In) {
        if (Variable == Rop) {
          rval = Value;
          break;
        }
      }
    }

    // handle Top or Bottom as a operand
    if (std::holds_alternative<Top>(lval) ||
        std::holds_alternative<Top>(rval) ||
        std::holds_alternative<Bottom>(lval) ||
        std::holds_alternative<Bottom>(rval)) {

      Out.insert({Op, Bottom{}});
      return Out;

    } else {
      Out.insert({S, executeBinOperation(Op->getOpcode(),
                                         *std::get_if<plain_d_t>(&lval),
                                         *std::get_if<plain_d_t>(&rval))});
      return Out;
    }
  }

  return Out;
}

LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>
IntraMonoFullConstantPropagation::executeBinOperation(
    const unsigned Op, IntraMonoFullConstantPropagation::plain_d_t Lop,
    IntraMonoFullConstantPropagation::plain_d_t Rop) {
  // default initialize with BOTTOM (all information)
  LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> Res = Bottom{};
  switch (Op) {
  case llvm::Instruction::Add:
    Res = Lop + Rop;
    break;

  case llvm::Instruction::Sub:
    Res = Lop - Rop;
    break;

  case llvm::Instruction::Mul:
    Res = Lop * Rop;
    break;

  case llvm::Instruction::UDiv:
  case llvm::Instruction::SDiv:
    Res = Lop / Rop;
    break;

  case llvm::Instruction::URem:
  case llvm::Instruction::SRem:
    Res = Lop % Rop;
    break;

  default:
    break;
  }
  return Res;
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
