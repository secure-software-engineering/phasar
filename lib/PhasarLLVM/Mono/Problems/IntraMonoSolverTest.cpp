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
#include <iostream>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>
#include <phasar/Utils/LLVMShorthands.h>

#include <phasar/PhasarLLVM/Mono/Problems/IntraMonoSolverTest.h>
using namespace std;
using namespace psr;

namespace psr {

IntraMonoSolverTest::IntraMonoSolverTest(IntraMonoSolverTest::CFG_t Cfg,
                                         IntraMonoSolverTest::Method_t F)
    : IntraMonoProblem<
          IntraMonoSolverTest::Node_t, IntraMonoSolverTest::Domain_t,
          IntraMonoSolverTest::Method_t, IntraMonoSolverTest::CFG_t>(Cfg, F) {}

MonoSet<IntraMonoSolverTest::Domain_t>
IntraMonoSolverTest::join(const MonoSet<IntraMonoSolverTest::Domain_t> &Lhs,
                          const MonoSet<IntraMonoSolverTest::Domain_t> &Rhs) {
  cout << "IntraMonoSolverTest::join()\n";
  MonoSet<IntraMonoSolverTest::Domain_t> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool IntraMonoSolverTest::sqSubSetEqual(
    const MonoSet<IntraMonoSolverTest::Domain_t> &Lhs,
    const MonoSet<IntraMonoSolverTest::Domain_t> &Rhs) {
  cout << "IntraMonoSolverTest::sqSubSetEqual()\n";
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<IntraMonoSolverTest::Domain_t>
IntraMonoSolverTest::flow(IntraMonoSolverTest::Node_t S,
                          const MonoSet<IntraMonoSolverTest::Domain_t> &In) {
  cout << "IntraMonoSolverTest::flow()\n";
  MonoSet<IntraMonoSolverTest::Domain_t> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Store = llvm::dyn_cast<llvm::StoreInst>(S)) {
    Result.insert(Store);
  }
  return Result;
}

MonoMap<IntraMonoSolverTest::Node_t, MonoSet<IntraMonoSolverTest::Domain_t>>
IntraMonoSolverTest::initialSeeds() {
  cout << "IntraMonoSolverTest::initialSeeds()\n";
  return {};
}

void IntraMonoSolverTest::printNode(ostream &os,
                                    IntraMonoSolverTest::Node_t n) const {
  os << llvmIRToString(n);
}

void IntraMonoSolverTest::printDataFlowFact(
    ostream &os, IntraMonoSolverTest::Domain_t d) const {
  os << llvmIRToString(d);
}

void IntraMonoSolverTest::printMethod(ostream &os,
                                      IntraMonoSolverTest::Method_t m) const {
  os << m->getName().str();
}

} // namespace psr
