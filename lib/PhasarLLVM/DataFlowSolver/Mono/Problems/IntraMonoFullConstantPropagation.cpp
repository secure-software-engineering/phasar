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
#include <utility>

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

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
    std::ostream &OS, const llvm::Instruction *N) const {
  OS << llvmIRToString(N);
}

void IntraMonoFullConstantPropagation::printDataFlowFact(
    std::ostream &OS, std::pair<const llvm::Value *, unsigned> D) const {
  OS << "< " + llvmIRToString(D.first)
     << ", " + std::to_string(D.second) + " >";
}

void IntraMonoFullConstantPropagation::printFunction(
    std::ostream &OS, const llvm::Function *M) const {
  OS << M->getName().str();
}

} // namespace psr
