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

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace std {
template <> struct hash<pair<const llvm::Value *, unsigned>> {
  size_t operator()(const pair<const llvm::Value *, unsigned> &p) const {
    std::hash<const llvm::Value *> hash_ptr;
    std::hash<unsigned> hash_unsigned;
    size_t hp = hash_ptr(p.first);
    size_t hu = hash_unsigned(p.second);
    return hp ^ (hu << 1);
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
                       IntraMonoFullConstantPropagation::i_t>(IRDB, TH, CF, PT,
                                                              EntryPoints) {}

BitVectorSet<std::pair<const llvm::Value *, unsigned>>
IntraMonoFullConstantPropagation::join(
    const BitVectorSet<std::pair<const llvm::Value *, unsigned>> &Lhs,
    const BitVectorSet<std::pair<const llvm::Value *, unsigned>> &Rhs) {
  return Lhs.setUnion(Rhs);
}

bool IntraMonoFullConstantPropagation::sqSubSetEqual(
    const BitVectorSet<std::pair<const llvm::Value *, unsigned>> &Lhs,
    const BitVectorSet<std::pair<const llvm::Value *, unsigned>> &Rhs) {
  return Lhs.includes(Rhs);
}

BitVectorSet<std::pair<const llvm::Value *, unsigned>>
IntraMonoFullConstantPropagation::normalFlow(
    const llvm::Instruction *S,
    const BitVectorSet<std::pair<const llvm::Value *, unsigned>> &In) {
  return BitVectorSet<std::pair<const llvm::Value *, unsigned>>();
}

std::unordered_map<const llvm::Instruction *,
                   BitVectorSet<std::pair<const llvm::Value *, unsigned>>>
IntraMonoFullConstantPropagation::initialSeeds() {
  return std::unordered_map<
      const llvm::Instruction *,
      BitVectorSet<std::pair<const llvm::Value *, unsigned>>>();
}

void IntraMonoFullConstantPropagation::printNode(
    std::ostream &os, const llvm::Instruction *n) const {
  os << llvmIRToString(n);
}

void IntraMonoFullConstantPropagation::printDataFlowFact(
    std::ostream &os, std::pair<const llvm::Value *, unsigned> d) const {
  os << "< " + llvmIRToString(d.first)
     << ", " + std::to_string(d.second) + " >";
}

void IntraMonoFullConstantPropagation::printFunction(
    std::ostream &os, const llvm::Function *m) const {
  os << m->getName().str();
}

} // namespace psr
