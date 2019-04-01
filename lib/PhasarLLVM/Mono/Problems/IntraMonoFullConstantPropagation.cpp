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
#include <phasar/Utils/LLVMShorthands.h>

#include <phasar/PhasarLLVM/Mono/Problems/IntraMonoFullConstantPropagation.h>

using namespace std;
using namespace psr;
namespace psr {

IntraMonoFullConstantPropagation::IntraMonoFullConstantPropagation(
    LLVMBasedCFG &Cfg, m_t F)
    : IntraMonoProblem<n_t, d_t, m_t, LLVMBasedCFG &>(Cfg, F) {}

MonoSet<d_t> IntraMonoFullConstantPropagation::join(const MonoSet<d_t> &Lhs,
                                                    const MonoSet<d_t> &Rhs) {
  MonoSet<d_t> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool IntraMonoFullConstantPropagation::sqSubSetEqual(const MonoSet<d_t> &Lhs,
                                                     const MonoSet<d_t> &Rhs) {
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<d_t> IntraMonoFullConstantPropagation::normalFlow(n_t S,
                                                    const MonoSet<d_t> &In) {
  return MonoSet<d_t>();
}

MonoMap<n_t, MonoSet<d_t>> IntraMonoFullConstantPropagation::initialSeeds() {
  return MonoMap<n_t, MonoSet<d_t>>();
}

void IntraMonoFullConstantPropagation::printNode(ostream &os, n_t n) const {
  os << llvmIRToString(n);
}

void IntraMonoFullConstantPropagation::printDataFlowFact(ostream &os,
                                                         d_t d) const {
  os << "< " + llvmIRToString(d.first) << ", " + to_string(d.second) + " >";
}

void IntraMonoFullConstantPropagation::printMethod(ostream &os, m_t m) const {
  os << m->getName().str();
}

} // namespace psr
