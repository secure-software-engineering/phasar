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
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
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
    : IntraMonoProblem<IntraMonoFullConstantPropagationAnalysisDomain>(
          IRDB, TH, CF, PT, std::move(EntryPoints)) {}

IntraMonoFullConstantPropagation::mono_container_t
IntraMonoFullConstantPropagation::merge(
    const IntraMonoFullConstantPropagation::mono_container_t &Lhs,
    const IntraMonoFullConstantPropagation::mono_container_t &Rhs) {
  IntraMonoFullConstantPropagation::mono_container_t Out;
  for (const auto &[Key, Value] : Lhs) {
    auto Search = Rhs.find(Key);
    if (Search != Rhs.end() && Value == Search->second) {
      Out.insert({Key, Value});
    }
  }
  return Out;
}

bool IntraMonoFullConstantPropagation::equal_to(
    const IntraMonoFullConstantPropagation::mono_container_t &Lhs,
    const IntraMonoFullConstantPropagation::mono_container_t &Rhs) {
  return Rhs == Lhs;
}

std::unordered_map<IntraMonoFullConstantPropagation::n_t,
                   IntraMonoFullConstantPropagation::mono_container_t>
IntraMonoFullConstantPropagation::initialSeeds() {
  std::unordered_map<IntraMonoFullConstantPropagation::n_t,
                     IntraMonoFullConstantPropagation::mono_container_t>
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

IntraMonoFullConstantPropagation::mono_container_t
IntraMonoFullConstantPropagation::normalFlow(
    IntraMonoFullConstantPropagation::n_t Inst,
    const IntraMonoFullConstantPropagation::mono_container_t &In) {
  auto Out = In;

  // check Alloca instructions
  if (auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(Inst)) {
    if (Alloc->getAllocatedType()->isIntegerTy()) {
      Out.insert({Alloc, Top{}});
    }
  }

  // check store instructions
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(Inst)) {
    auto ValueOp = Store->getValueOperand();
    // Case I: Integer literal
    if (auto val = llvm::dyn_cast<llvm::ConstantInt>(ValueOp)) {
      auto Search = In.find(Store->getPointerOperand());
      if (Search != In.end() &&
          !std::holds_alternative<Bottom>(Search->second)) {
        Out[Store->getPointerOperand()] = val->getSExtValue();
        return Out;
      }
    }
    // Case II: Storing an integer typed value
    if (ValueOp->getType()->isIntegerTy()) {
      // get value to be stored
      LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> latticeVal =
          Top{};
      if (In.find(ValueOp) != In.end()) {
        latticeVal = In.at(ValueOp);
      }
      // store value in variable if it is not top
      if (!std::holds_alternative<Top>(latticeVal)) {
        auto Search = In.find(Store->getPointerOperand());
        if (Search != In.end() &&
            !std::holds_alternative<Bottom>(Search->second)) {
          Out[Store->getPointerOperand()] = latticeVal;
          return Out;
        }
      }
    }
  }

  // check load instructions
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(Inst)) {
    auto Search = In.find(Load->getPointerOperand());
    if (Search != In.end()) {
      Out[Load] = Search->second;
      return Out;
    }
  }

  // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
  if (auto Op = llvm::dyn_cast<llvm::BinaryOperator>(Inst)) {
    auto Lop = Inst->getOperand(0);
    auto Rop = Inst->getOperand(1);
    LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> lval;
    LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> rval;

    // get first operand value
    if (auto val = llvm::dyn_cast<llvm::ConstantInt>(Lop)) {
      lval = val->getSExtValue();
    } else {
      auto Search = In.find(Lop);
      if (Search != In.end()) {
        lval = Search->second;
      }
    }

    // get second operand value
    if (auto val = llvm::dyn_cast<llvm::ConstantInt>(Rop)) {
      rval = val->getSExtValue();
    } else {
      auto Search = In.find(Rop);
      if (Search != In.end()) {
        rval = Search->second;
      }
    }

    // handle Top or Bottom as a operand
    if (std::holds_alternative<Top>(lval) ||
        std::holds_alternative<Top>(rval) ||
        std::holds_alternative<Bottom>(lval) ||
        std::holds_alternative<Bottom>(rval)) {

      Out[Op] = Bottom{};
      return Out;

    } else {
      Out[Op] =
          executeBinOperation(Op->getOpcode(), *std::get_if<plain_d_t>(&lval),
                              *std::get_if<plain_d_t>(&rval));
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
    std::ostream &OS, IntraMonoFullConstantPropagation::n_t Inst) const {
  OS << llvmIRToString(Inst);
}

void IntraMonoFullConstantPropagation::printDataFlowFact(
    std::ostream &OS, IntraMonoFullConstantPropagation::d_t Fact) const {
  OS << "< " + llvmIRToString(Fact.first) << ", ";
  if (std::holds_alternative<Top>(Fact.second)) {
    OS << std::get<Top>(Fact.second);
  }
  if (std::holds_alternative<Bottom>(Fact.second)) {
    OS << std::get<Bottom>(Fact.second);
  }
  if (std::holds_alternative<IntraMonoFullConstantPropagation::plain_d_t>(
          Fact.second)) {
    OS << std::get<IntraMonoFullConstantPropagation::plain_d_t>(Fact.second);
  }
  OS << " >\n";
}

void IntraMonoFullConstantPropagation::printFunction(
    std::ostream &OS, IntraMonoFullConstantPropagation::f_t Fun) const {
  OS << Fun->getName().str();
}

} // namespace psr
