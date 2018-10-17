/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonoSolverTest.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Macros.h>

using namespace std;
using namespace psr;

namespace psr {

using Node_t = InterMonoSolverTest::Node_t;
using Domain_t = InterMonoSolverTest::Domain_t;
using Method_t = InterMonoSolverTest::Method_t;
using ICFG_t = InterMonoSolverTest::ICFG_t;

InterMonoSolverTest::InterMonoSolverTest(ICFG_t &Icfg,
                                         vector<string> EntryPoints)
    : InterMonoProblem<Node_t, Domain_t, Method_t, ICFG_t>(Icfg),
      EntryPoints(EntryPoints) {}

Domain_t InterMonoSolverTest::join(const Domain_t &Lhs, const Domain_t &Rhs) {
  // cout << "InterMonoSolverTest::join()\n";
  Domain_t Result;
  // set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
  //           inserter(Result, Result.begin()));
  return Result;
}

bool InterMonoSolverTest::sqSubSetEqual(const Domain_t &Lhs,
                                        const Domain_t &Rhs) {
  // cout << "InterMonoSolverTest::sqSubSetEqual()\n";
  // return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
  return true;
}

Domain_t InterMonoSolverTest::normalFlow(const Node_t Stmt,
                                         const Domain_t &In) {
  cout << "InterMonoSolverTest::normalFlow()\n";
  // MonoSet<const llvm::Value *> Result;
  // Result.insert(In.begin(), In.end());
  // if (const auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(Stmt)) {
  //   Result.insert(Alloc);
  // }
  return In;
}

Domain_t InterMonoSolverTest::callFlow(const Node_t CallSite,
                                       const Method_t Callee,
                                       const Domain_t &In) {
  cout << "InterMonoSolverTest::callFlow()\n";
  // MonoSet<const llvm::Value *> Result;
  // Result.insert(In.begin(), In.end());
  // if (const auto Call = llvm::dyn_cast<llvm::CallInst>(CallSite)) {
  //   Result.insert(Call);
  // }
  return In;
}

Domain_t InterMonoSolverTest::returnFlow(const Node_t CallSite,
                                         const Method_t Callee,
                                         const Node_t RetSite,
                                         const Domain_t &In) {
  cout << "InterMonoSolverTest::returnFlow()\n";
  return In;
}

Domain_t InterMonoSolverTest::callToRetFlow(const Node_t CallSite,
                                            const Node_t RetSite,
                                            const Domain_t &In) {
  cout << "InterMonoSolverTest::callToRetFlow()\n";
  return In;
}

MonoMap<Node_t, Domain_t> InterMonoSolverTest::initialSeeds() {
  cout << "InterMonoSolverTest::initialSeeds()\n";
  const Method_t main = ICFG.getMethod("main");
  MonoMap<Node_t, Domain_t> Seeds;
  Seeds.insert(make_pair(&main->front().front(), Domain_t()));
  return Seeds;
}

void InterMonoSolverTest::printNode(ostream &os, Node_t n) const {
  os << llvmIRToString(n);
}

void InterMonoSolverTest::printDataFlowFact(ostream &os, Domain_t d) const {
  for (auto fact : d) {
    os << llvmIRToString(fact) << '\n';
  }
}

void InterMonoSolverTest::printMethod(ostream &os, Method_t m) const {
  os << m->getName().str();
}

bool InterMonoSolverTest::recompute(const Method_t Callee) { return false; }

} // namespace psr
