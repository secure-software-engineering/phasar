/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <ostream>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>

#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoFullConstantPropagation.h>

using namespace std;
using namespace psr;
namespace psr {

IntraMonoFullConstantPropagation::IntraMonoFullConstantPropagation(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedCFG *CF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IntraMonoProblem<IntraMonoFullConstantPropagation::n_t,
                       IntraMonoFullConstantPropagation::d_t,
                       IntraMonoFullConstantPropagation::m_t,
                       IntraMonoFullConstantPropagation::t_t,
                       IntraMonoFullConstantPropagation::v_t,
                       IntraMonoFullConstantPropagation::i_t>(IRDB, TH, CF, PT,
                                                              EntryPoints) {}

MonoSet<std::pair<const llvm::Value *, unsigned>>
IntraMonoFullConstantPropagation::join(
    const MonoSet<std::pair<const llvm::Value *, unsigned>> &Lhs,
    const MonoSet<std::pair<const llvm::Value *, unsigned>> &Rhs) {
  MonoSet<std::pair<const llvm::Value *, unsigned>> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool IntraMonoFullConstantPropagation::sqSubSetEqual(
    const MonoSet<std::pair<const llvm::Value *, unsigned>> &Lhs,
    const MonoSet<std::pair<const llvm::Value *, unsigned>> &Rhs) {
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<std::pair<const llvm::Value *, unsigned>>
IntraMonoFullConstantPropagation::normalFlow(
    const llvm::Instruction *S,
    const MonoSet<std::pair<const llvm::Value *, unsigned>> &In) {
  return MonoSet<std::pair<const llvm::Value *, unsigned>>();
}

MonoMap<const llvm::Instruction *,
        MonoSet<std::pair<const llvm::Value *, unsigned>>>
IntraMonoFullConstantPropagation::initialSeeds() {
  return MonoMap<const llvm::Instruction *,
                 MonoSet<std::pair<const llvm::Value *, unsigned>>>();
}

void IntraMonoFullConstantPropagation::printNode(
    ostream &os, const llvm::Instruction *n) const {
  os << llvmIRToString(n);
}

void IntraMonoFullConstantPropagation::printDataFlowFact(
    ostream &os, std::pair<const llvm::Value *, unsigned> d) const {
  os << "< " + llvmIRToString(d.first)
     << ", " + std::to_string(d.second) + " >";
}

void IntraMonoFullConstantPropagation::printMethod(
    ostream &os, const llvm::Function *m) const {
  os << m->getName().str();
}

} // namespace psr
