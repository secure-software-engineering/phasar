/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MonoSolverTest.cpp
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#include <algorithm>
#include <iosfwd>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>
#include <phasar/Utils/LLVMShorthands.h>

#include <phasar/PhasarLLVM/Mono/Problems/IntraMonoSolverTest.h>
using namespace std;
using namespace psr;

namespace psr {

IntraMonoSolverTest::IntraMonoSolverTest(i_t Cfg, m_t F)
    : IntraMonoProblem<n_t, d_t, m_t, i_t>(Cfg, F) {}

MonoSet<d_t> IntraMonoSolverTest::join(const MonoSet<d_t> &Lhs,
                                       const MonoSet<d_t> &Rhs) {
  cout << "IntraMonoSolverTest::join()\n";
  MonoSet<d_t> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool IntraMonoSolverTest::sqSubSetEqual(const MonoSet<d_t> &Lhs,
                                        const MonoSet<d_t> &Rhs) {
  cout << "IntraMonoSolverTest::sqSubSetEqual()\n";
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<d_t> IntraMonoSolverTest::normalFlow(n_t S, const MonoSet<d_t> &In) {
  cout << "IntraMonoSolverTest::normalFlow()\n";
  MonoSet<d_t> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Store = llvm::dyn_cast<llvm::StoreInst>(S)) {
    Result.insert(Store);
  }
  return Result;
}

MonoMap<n_t, MonoSet<d_t>> IntraMonoSolverTest::initialSeeds() {
  cout << "IntraMonoSolverTest::initialSeeds()\n";
  return {};
}

void IntraMonoSolverTest::printNode(ostream &os, n_t n) const {
  os << llvmIRToString(n);
}

void IntraMonoSolverTest::printDataFlowFact(ostream &os, d_t d) const {
  os << llvmIRToString(d);
}

void IntraMonoSolverTest::printMethod(ostream &os, m_t m) const {
  os << m->getName().str();
}

} // namespace psr
