/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <llvm/IR/Constants.h>
#include <ostream>
#include <utility>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoUninitVariables.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/BitVectorSet.h>
#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;
namespace psr {

IntraMonoUninitVariables::IntraMonoUninitVariables(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedCFG *CF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IntraMonoProblem<
          IntraMonoUninitVariables::n_t, IntraMonoUninitVariables::d_t,
          IntraMonoUninitVariables::f_t, IntraMonoUninitVariables::t_t,
          IntraMonoUninitVariables::v_t, IntraMonoUninitVariables::i_t,
          IntraMonoUninitVariables::container_t>(IRDB, TH, CF, PT,
                                                 std::move(EntryPoints)) {}

IntraMonoUninitVariables::container_t IntraMonoUninitVariables::merge(
    const IntraMonoUninitVariables::container_t &Lhs,
    const IntraMonoUninitVariables::container_t &Rhs) {
  IntraMonoUninitVariables::container_t Intersect;
  std::set_intersection(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
                        std::inserter(Intersect, Intersect.begin()));
  return Intersect;
}

bool IntraMonoUninitVariables::equal_to(
    const IntraMonoUninitVariables::container_t &Lhs,
    const IntraMonoUninitVariables::container_t &Rhs) {
  return Lhs == Rhs;
}

IntraMonoUninitVariables::container_t IntraMonoUninitVariables::allTop() {
  return {};
}

IntraMonoUninitVariables::container_t IntraMonoUninitVariables::normalFlow(
    const llvm::Instruction *S,
    const IntraMonoUninitVariables::container_t &In) {
  auto Out = In;
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(S)) {
    Out.insert(Alloca);
  }
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(S)) {
    if (Store->getValueOperand()->getType()->isIntegerTy() &&
        llvm::isa<llvm::ConstantData>(Store->getValueOperand())) {
      llvm::outs() << "Found initialization at: ";
      Store->print(llvm::outs());
      llvm::outs() << '\n';
      Out.erase(Store->getPointerOperand());
    }
  }
  return Out;
}

unordered_map<const llvm::Instruction *, IntraMonoUninitVariables::container_t>
IntraMonoUninitVariables::initialSeeds() {
  std::unordered_map<const llvm::Instruction *,
                     IntraMonoUninitVariables::container_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    if (const auto *Fun = IRDB->getFunctionDefinition(EntryPoint)) {
      for (const auto *I : CF->getStartPointsOf(Fun)) {
        Seeds[I] = allTop();
      }
    }
  }
  return Seeds;
}

void IntraMonoUninitVariables::printNode(ostream &os,
                                         const llvm::Instruction *n) const {
  os << llvmIRToString(n);
}

void IntraMonoUninitVariables::printDataFlowFact(
    ostream &os, IntraMonoUninitVariables::d_t d) const {
  os << llvmIRToString(d);
}

void IntraMonoUninitVariables::printFunction(ostream &os,
                                             const llvm::Function *m) const {
  os << m->getName().str();
}

} // namespace psr
