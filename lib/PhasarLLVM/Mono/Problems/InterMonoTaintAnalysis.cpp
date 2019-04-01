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
#include <phasar/PhasarLLVM/Mono/Problems/InterMonoTaintAnalysis.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Macros.h>

using namespace std;
using namespace psr;

namespace psr {

InterMonoTaintAnalysis::InterMonoTaintAnalysis(i_t Icfg,
                                               vector<string> EntryPoints)
    : InterMonoProblem<n_t, d_t, m_t, i_t>(Icfg),
      EntryPoints(EntryPoints) {}

MonoSet<d_t> InterMonoTaintAnalysis::join(const MonoSet<d_t> &Lhs,
                                      const MonoSet<d_t> &Rhs) {
  cout << "InterMonoTaintAnalysis::join()\n";
  MonoSet<d_t> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  cout << "Result size: " << Result.size() << endl;
  return Result;
}

bool InterMonoTaintAnalysis::sqSubSetEqual(const MonoSet<d_t> &Lhs,
                                           const MonoSet<d_t> &Rhs) {
  cout << "InterMonoTaintAnalysis::sqSubSetEqual()\n";
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
  // return true;
}

MonoSet<d_t> InterMonoTaintAnalysis::normalFlow(const n_t Stmt,
                                            const MonoSet<d_t> &In) {
  cout << "InterMonoTaintAnalysis::normalFlow()\n";
  MonoSet<d_t> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(Stmt)) {
    Result.insert(Alloc);
  }
  cout << "Result size: " << Result.size() << endl;
  return Result;
}

MonoSet<d_t> InterMonoTaintAnalysis::callFlow(const n_t CallSite,
                                          const m_t Callee,
                                          const MonoSet<d_t> &In) {
  cout << "InterMonoTaintAnalysis::callFlow()\n";
  MonoSet<d_t> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Call = llvm::dyn_cast<llvm::CallInst>(CallSite)) {
    Result.insert(Call);
  }
  cout << "Result size: " << Result.size() << endl;
  return Result;
}

MonoSet<d_t> InterMonoTaintAnalysis::returnFlow(const n_t CallSite,
                                            const m_t Callee,
                                            const n_t RetSite,
                                            const MonoSet<d_t> &In) {
  cout << "InterMonoTaintAnalysis::returnFlow()\n";
  return In;
}

MonoSet<d_t> InterMonoTaintAnalysis::callToRetFlow(const n_t CallSite,
                                               const n_t RetSite,
                                               const MonoSet<d_t> &In) {
  cout << "InterMonoTaintAnalysis::callToRetFlow()\n";
  return In;
}

MonoMap<n_t, MonoSet<d_t>> InterMonoTaintAnalysis::initialSeeds() {
  cout << "InterMonoTaintAnalysis::initialSeeds()\n";
  const m_t main = ICFG.getMethod("main");
  MonoMap<n_t, MonoSet<d_t>> Seeds;
  Seeds.insert(make_pair(&main->front().front(), MonoSet<d_t>()));
  return Seeds;
}

void InterMonoTaintAnalysis::printNode(ostream &os, n_t n) const {
  os << llvmIRToString(n);
}

void InterMonoTaintAnalysis::printDataFlowFact(ostream &os, d_t d) const {
  os << llvmIRToString(fact) << '\n';
}

void InterMonoTaintAnalysis::printMethod(ostream &os, m_t m) const {
  os << m->getName().str();
}

} // namespace psr
