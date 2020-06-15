/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann and others
 *****************************************************************************/

#include "llvm/Support/Casting.h"
#include <algorithm>
#include <ostream>

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/LLVMShorthands.h"
#include <unordered_map>
#include <utility>

using namespace std;
using namespace psr;

namespace psr {

InterMonoFullConstantPropagation::InterMonoFullConstantPropagation(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : InterMonoProblem<InterMonoFullConstantPropagationAnalysisDomain>(
          IRDB, TH, ICF, PT, std::move(EntryPoints)) {}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::join(
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Rhs) {
  // TODO implement
  return {};
}

bool InterMonoFullConstantPropagation::sqSubSetEqual(
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Rhs) {
  // TODO implement
  return true;
}

std::unordered_map<InterMonoFullConstantPropagation::n_t,
                   BitVectorSet<InterMonoFullConstantPropagation::d_t>>
InterMonoFullConstantPropagation::initialSeeds() {
  std::unordered_map<InterMonoFullConstantPropagation::n_t,
                     BitVectorSet<InterMonoFullConstantPropagation::d_t>>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    if (const auto *Fun = IRDB->getFunctionDefinition(EntryPoint)) {
      auto Is = ICF->getStartPointsOf(Fun);
      for (const auto *I : Is) {
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
  // TODO finish implementation
  auto Out = In;
  if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(S)) {
    if (Alloc->getAllocatedType()->isIntegerTy()) {
      Out.insert({Alloc, Top{}});
    }
  }
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(S)) {
    if (Store->getValueOperand()->getType()->isIntegerTy()) {
      // ...
    }
  }
  return Out;
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::callFlow(
    InterMonoFullConstantPropagation::n_t CallSite,
    InterMonoFullConstantPropagation::f_t Callee,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &In) {
  // TODO implement
  return In;
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::returnFlow(
    InterMonoFullConstantPropagation::n_t CallSite,
    InterMonoFullConstantPropagation::f_t Callee,
    InterMonoFullConstantPropagation::n_t ExitStmt,
    InterMonoFullConstantPropagation::n_t RetSite,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &In) {
  // TODO implement
  return In;
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

void InterMonoFullConstantPropagation::printNode(
    std::ostream &OS, InterMonoFullConstantPropagation::n_t N) const {
  OS << llvmIRToString(N);
}

void InterMonoFullConstantPropagation::printDataFlowFact(
    std::ostream &OS, InterMonoFullConstantPropagation::d_t D) const {
  OS << "< " + llvmIRToString(D.first) << ", ";
  if (std::holds_alternative<Top>(D.second)) {
    OS << std::get<Top>(D.second);
  }
  if (std::holds_alternative<Bottom>(D.second)) {
    OS << std::get<Bottom>(D.second);
  }
  if (std::holds_alternative<InterMonoFullConstantPropagation::plain_d_t>(
          D.second)) {
    OS << std::get<InterMonoFullConstantPropagation::plain_d_t>(D.second);
  }
  OS << " >";
}

void InterMonoFullConstantPropagation::printFunction(
    std::ostream &OS, InterMonoFullConstantPropagation::f_t F) const {
  OS << F->getName().str();
}

} // namespace psr
