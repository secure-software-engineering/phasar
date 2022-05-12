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
  for (const auto &EntryPoint : EntryPoints) {
    if (const auto *Fun = IRDB->getFunctionDefinition(EntryPoint)) {
      auto Is = CF->getStartPointsOf(Fun);
      for (const auto *I : Is) {
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
  if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(Inst)) {
    if (Alloc->getAllocatedType()->isIntegerTy()) {
      Out.insert({Alloc, Top{}});
    }
  }

  // check store instructions
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Inst)) {
    const auto *ValueOp = Store->getValueOperand();
    // Case I: Integer literal
    if (const auto *Val = llvm::dyn_cast<llvm::ConstantInt>(ValueOp)) {
      auto Search = In.find(Store->getPointerOperand());
      if (Search != In.end() &&
          !std::holds_alternative<Bottom>(Search->second)) {
        Out[Store->getPointerOperand()] = Val->getSExtValue();
        return Out;
      }
    }
    // Case II: Storing an integer typed value
    if (ValueOp->getType()->isIntegerTy()) {
      // get value to be stored
      LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> LatticeVal =
          Top{};
      if (In.find(ValueOp) != In.end()) {
        LatticeVal = In.at(ValueOp);
      }
      // store value in variable if it is not top
      if (!std::holds_alternative<Top>(LatticeVal)) {
        auto Search = In.find(Store->getPointerOperand());
        if (Search != In.end() &&
            !std::holds_alternative<Bottom>(Search->second)) {
          Out[Store->getPointerOperand()] = LatticeVal;
          return Out;
        }
      }
    }
  }

  // check load instructions
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Inst)) {
    auto Search = In.find(Load->getPointerOperand());
    if (Search != In.end()) {
      Out[Load] = Search->second;
      return Out;
    }
  }

  // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
  if (const auto *Op = llvm::dyn_cast<llvm::BinaryOperator>(Inst)) {
    auto *Lop = Inst->getOperand(0);
    auto *Rop = Inst->getOperand(1);
    LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> LeftVal;
    LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t> RightVal;

    // get first operand value
    if (const auto *Val = llvm::dyn_cast<llvm::ConstantInt>(Lop)) {
      LeftVal = Val->getSExtValue();
    } else {
      auto Search = In.find(Lop);
      if (Search != In.end()) {
        LeftVal = Search->second;
      }
    }

    // get second operand value
    if (const auto *Val = llvm::dyn_cast<llvm::ConstantInt>(Rop)) {
      RightVal = Val->getSExtValue();
    } else {
      auto Search = In.find(Rop);
      if (Search != In.end()) {
        RightVal = Search->second;
      }
    }

    // handle Top or Bottom as a operand
    if (std::holds_alternative<Top>(LeftVal) ||
        std::holds_alternative<Top>(RightVal) ||
        std::holds_alternative<Bottom>(LeftVal) ||
        std::holds_alternative<Bottom>(RightVal)) {

      Out[Op] = Bottom{};
      return Out;
    }
    Out[Op] =
        executeBinOperation(Op->getOpcode(), *std::get_if<plain_d_t>(&LeftVal),
                            *std::get_if<plain_d_t>(&RightVal));
    return Out;
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
    if (Rop != 0) {
      Res = Lop / Rop;
    }
    break;

  case llvm::Instruction::URem:
  case llvm::Instruction::SRem:
    if (Rop != 0) {
      Res = Lop % Rop;
    }
    break;

  default:
    break;
  }
  return Res;
}

void IntraMonoFullConstantPropagation::printNode(
    llvm::raw_ostream &OS, IntraMonoFullConstantPropagation::n_t Inst) const {
  OS << llvmIRToString(Inst);
}

void IntraMonoFullConstantPropagation::printDataFlowFact(
    llvm::raw_ostream &OS, IntraMonoFullConstantPropagation::d_t Fact) const {
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
    llvm::raw_ostream &OS, IntraMonoFullConstantPropagation::f_t Fun) const {
  OS << Fun->getName();
}

} // namespace psr
