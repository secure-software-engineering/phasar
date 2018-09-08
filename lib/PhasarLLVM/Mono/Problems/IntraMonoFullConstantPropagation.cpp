/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <iostream>

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
    LLVMBasedCFG &Cfg, IntraMonoFullConstantPropagation::Method_t F)
    : IntraMonoProblem<IntraMonoFullConstantPropagation::Node_t,
                       IntraMonoFullConstantPropagation::Domain_t,
                       IntraMonoFullConstantPropagation::Method_t,
                       LLVMBasedCFG &>(Cfg, F) {}

MonoSet<IntraMonoFullConstantPropagation::Domain_t>
IntraMonoFullConstantPropagation::join(
    const MonoSet<IntraMonoFullConstantPropagation::Domain_t> &Lhs,
    const MonoSet<IntraMonoFullConstantPropagation::Domain_t> &Rhs) {
  MonoSet<IntraMonoFullConstantPropagation::Domain_t> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool IntraMonoFullConstantPropagation::sqSubSetEqual(
    const MonoSet<IntraMonoFullConstantPropagation::Domain_t> &Lhs,
    const MonoSet<IntraMonoFullConstantPropagation::Domain_t> &Rhs) {
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<IntraMonoFullConstantPropagation::Domain_t>
IntraMonoFullConstantPropagation::flow(
    IntraMonoFullConstantPropagation::Node_t S,
    const MonoSet<IntraMonoFullConstantPropagation::Domain_t> &In) {
  return MonoSet<IntraMonoFullConstantPropagation::Domain_t>();
}

MonoMap<IntraMonoFullConstantPropagation::Node_t,
        MonoSet<IntraMonoFullConstantPropagation::Domain_t>>
IntraMonoFullConstantPropagation::initialSeeds() {
  return MonoMap<IntraMonoFullConstantPropagation::Node_t,
                 MonoSet<IntraMonoFullConstantPropagation::Domain_t>>();
}

void IntraMonoFullConstantPropagation::printNode(
    ostream &os, IntraMonoFullConstantPropagation::Node_t n) const {
  os << llvmIRToString(n);
}

void IntraMonoFullConstantPropagation::printDataFlowFact(
    ostream &os, IntraMonoFullConstantPropagation::Domain_t d) const {
  os << "< " + llvmIRToString(d.first) << ", " + to_string(d.second) + " >";
}

void IntraMonoFullConstantPropagation::printMethod(
    ostream &os, IntraMonoFullConstantPropagation::Method_t m) const {
  os << m->getName().str();
}

} // namespace psr
