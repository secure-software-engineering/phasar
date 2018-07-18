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

#include <phasar/PhasarLLVM/Mono/Problems/InterMonotoneSolverTest.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Macros.h>


using namespace std;
using namespace psr;

namespace psr {

using Node_t      = InterMonotoneSolverTest::Node_t;
using Domain_t    = InterMonotoneSolverTest::Domain_t;
using Method_t    = InterMonotoneSolverTest::Method_t;
using ICFG_t      = InterMonotoneSolverTest::ICFG_t;

InterMonotoneSolverTest::InterMonotoneSolverTest(ICFG_t &Icfg,
                                                 vector<string> EntryPoints)
    : InterMonotoneProblem<Node_t, Domain_t,
                           Method_t,
                           ICFG_t>(Icfg),
      EntryPoints(EntryPoints) {}

Domain_t
InterMonotoneSolverTest::join(const Domain_t &Lhs,
                              const Domain_t &Rhs) {
  // cout << "InterMonotoneSolverTest::join()\n";
  Domain_t Result;
  // set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
  //           inserter(Result, Result.begin()));
  return Result;
}

bool InterMonotoneSolverTest::sqSubSetEqual(
    const Domain_t &Lhs,
    const Domain_t &Rhs) {
  // cout << "InterMonotoneSolverTest::sqSubSetEqual()\n";
  // return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
  return true;
}

Domain_t
InterMonotoneSolverTest::normalFlow(const Node_t Stmt,
                                    const Domain_t &In) {
  cout << "InterMonotoneSolverTest::normalFlow()\n";
  // MonoSet<const llvm::Value *> Result;
  // Result.insert(In.begin(), In.end());
  // if (const auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(Stmt)) {
  //   Result.insert(Alloc);
  // }
  return In;
}

Domain_t
InterMonotoneSolverTest::callFlow(const Node_t CallSite,
                                  const Method_t Callee,
                                  const Domain_t &In) {
  cout << "InterMonotoneSolverTest::callFlow()\n";
  // MonoSet<const llvm::Value *> Result;
  // Result.insert(In.begin(), In.end());
  // if (const auto Call = llvm::dyn_cast<llvm::CallInst>(CallSite)) {
  //   Result.insert(Call);
  // }
  return In;
}

Domain_t InterMonotoneSolverTest::returnFlow(
    const Node_t CallSite, const Method_t Callee,
    const Node_t RetSite,
    const Domain_t &In) {
  cout << "InterMonotoneSolverTest::returnFlow()\n";
  return In;
}

Domain_t
InterMonotoneSolverTest::callToRetFlow(const Node_t CallSite,
                                       const Node_t RetSite,
                                       const Domain_t &In) {
  cout << "InterMonotoneSolverTest::callToRetFlow()\n";
  return In;
}

MonoMap<Node_t, Domain_t>
InterMonotoneSolverTest::initialSeeds() {
  cout << "InterMonotoneSolverTest::initialSeeds()\n";
  const Method_t main = ICFG.getMethod("main");
  MonoMap<Node_t, Domain_t> Seeds;
  Seeds.insert(
      std::make_pair(&main->front().front(), Domain_t()));
  return Seeds;
}

string InterMonotoneSolverTest::DtoString(const Domain_t d) {
  return "";
  // return llvmIRToString(d);
}

bool InterMonotoneSolverTest::recompute(const Method_t Callee) {
  return false;
}

} // namespace psr
