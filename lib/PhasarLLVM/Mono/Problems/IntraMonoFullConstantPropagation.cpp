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
#include <phasar/Utils/BitVectorSet.h>
#include <phasar/Utils/LLVMShorthands.h>

#include <phasar/PhasarLLVM/Mono/Problems/IntraMonoFullConstantPropagation.h>

using namespace std;
using namespace psr;
namespace psr {

IntraMonoFullConstantPropagation::IntraMonoFullConstantPropagation(
    LLVMBasedCFG &Cfg, const llvm::Function *F)
    : IntraMonoProblem<const llvm::Instruction *,
                       std::pair<const llvm::Value *, unsigned>,
                       const llvm::Function *, LLVMBasedCFG &>(Cfg, F) {}

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

unordered_map<const llvm::Instruction *,
              BitVectorSet<std::pair<const llvm::Value *, unsigned>>>
IntraMonoFullConstantPropagation::initialSeeds() {
  return unordered_map<
      const llvm::Instruction *,
      BitVectorSet<std::pair<const llvm::Value *, unsigned>>>();
}

void IntraMonoFullConstantPropagation::printNode(
    ostream &os, const llvm::Instruction *n) const {
  os << llvmIRToString(n);
}

void IntraMonoFullConstantPropagation::printDataFlowFact(
    ostream &os, std::pair<const llvm::Value *, unsigned> d) const {
  os << "< " + llvmIRToString(d.first) << ", " + to_string(d.second) + " >";
}

void IntraMonoFullConstantPropagation::printMethod(
    ostream &os, const llvm::Function *m) const {
  os << m->getName().str();
}

} // namespace psr
