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
          IntraMonoUninitVariables::v_t, IntraMonoUninitVariables::i_t>(
          IRDB, TH, CF, PT, EntryPoints) {}

BitVectorSet<IntraMonoUninitVariables::d_t>
IntraMonoUninitVariables::merge(
    const BitVectorSet<IntraMonoUninitVariables::d_t> &Lhs,
    const BitVectorSet<IntraMonoUninitVariables::d_t> &Rhs) {
  return Lhs.setIntersect(Rhs);
}

bool IntraMonoUninitVariables::equal_to(
    const BitVectorSet<IntraMonoUninitVariables::d_t> &Lhs,
    const BitVectorSet<IntraMonoUninitVariables::d_t> &Rhs) {
  return Lhs == Rhs;
}

BitVectorSet<IntraMonoUninitVariables::d_t>
IntraMonoUninitVariables::normalFlow(
    const llvm::Instruction *S,
    const BitVectorSet<IntraMonoUninitVariables::d_t> &In) {
  BitVectorSet<IntraMonoUninitVariables::d_t> Out = In;
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(S)) {
    Out.insert(Alloca);
  }
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(S)) {
    if (Store->getValueOperand()->getType()->isIntegerTy()) {
        std::cout << "HERE I AM!\n";
        Store->getValueOperand()->print(llvm::outs());
        llvm::outs() << '\n';
        Out.erase(Store->getPointerOperand());
    }
  }
  return Out;
}

unordered_map<const llvm::Instruction *,
              BitVectorSet<IntraMonoUninitVariables::d_t>>
IntraMonoUninitVariables::initialSeeds() {
  std::unordered_map<const llvm::Instruction *,
                     BitVectorSet<IntraMonoUninitVariables::d_t>>
      Seeds;
  for (auto &EntryPoint : EntryPoints) {
    if (auto Fun = IRDB->getFunctionDefinition(EntryPoint)) {
      auto I = &Fun->front().front();
      Seeds[I] = {};
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
