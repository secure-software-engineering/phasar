/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iosfwd>

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

InterMonoSolverTest::InterMonoSolverTest(i_t &Icfg,
                                         vector<string> EntryPoints)
    : InterMonoProblem<n_t, d_t, m_t, i_t>(Icfg),
      EntryPoints(EntryPoints) {}

MonoSet<d_t> InterMonoSolverTest::join(const MonoSet<d_t> &Lhs, const MonoSet<d_t> &Rhs) {
  cout << "InterMonoSolverTest::join()\n";
  MonoSet<d_t> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool InterMonoSolverTest::sqSubSetEqual(const MonoSet<d_t> &Lhs,
                                        const MonoSet<d_t> &Rhs) {
  cout << "InterMonoSolverTest::sqSubSetEqual()\n";
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<d_t> InterMonoSolverTest::normalFlow(const n_t Stmt,
                                         const MonoSet<d_t> &In) {
  cout << "InterMonoSolverTest::normalFlow()\n";
  MonoSet<d_t> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(Stmt)) {
    Result.insert(Alloc);
  }
  return In;
}

MonoSet<d_t> InterMonoSolverTest::callFlow(const n_t CallSite,
                                       const m_t Callee,
                                       const MonoSet<d_t> &In) {
  cout << "InterMonoSolverTest::callFlow()\n";
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Call = llvm::dyn_cast<llvm::CallInst>(CallSite)) {
    Result.insert(Call);
  }
  return In;
}

MonoSet<d_t> InterMonoSolverTest::returnFlow(const n_t CallSite,
                                         const m_t Callee,
                                         const n_t RetSite,
                                         const MonoSet<d_t> &In) {
  cout << "InterMonoSolverTest::returnFlow()\n";
  return In;
}

MonoSet<d_t> InterMonoSolverTest::callToRetFlow(const n_t CallSite,
                                            const n_t RetSite,
                                            const MonoSet<d_t> &In) {
  cout << "InterMonoSolverTest::callToRetFlow()\n";
  return In;
}

MonoMap<n_t, MonoSet<d_t>> InterMonoSolverTest::initialSeeds() {
  cout << "InterMonoSolverTest::initialSeeds()\n";
  const m_t main = ICFG.getMethod("main");
  MonoMap<n_t, MonoSet<d_t>> Seeds;
  Seeds.insert(make_pair(&main->front().front(), MonoSet<d_t>()));
  return Seeds;
}

void InterMonoSolverTest::printNode(ostream &os, n_t n) const {
  os << llvmIRToString(n);
}

void InterMonoSolverTest::printDataFlowFact(ostream &os, d_t d) const { 
  os << llvmIRToString(fact) << '\n';
}

void InterMonoSolverTest::printMethod(ostream &os, m_t m) const {
  os << m->getName().str();
}


} // namespace psr
