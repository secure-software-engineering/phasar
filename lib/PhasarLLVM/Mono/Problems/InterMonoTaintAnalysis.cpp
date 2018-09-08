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
#include <phasar/PhasarLLVM/Mono/Problems/InterMonoTaintAnalysis.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Macros.h>

using namespace std;
using namespace psr;

namespace psr {

using Node_t = InterMonoTaintAnalysis::Node_t;
using Domain_t = InterMonoTaintAnalysis::Domain_t;
using Method_t = InterMonoTaintAnalysis::Method_t;
using ICFG_t = InterMonoTaintAnalysis::ICFG_t;

InterMonoTaintAnalysis::InterMonoTaintAnalysis(ICFG_t &Icfg,
                                               vector<string> EntryPoints)
    : InterMonoProblem<Node_t, Domain_t, Method_t, ICFG_t>(Icfg),
      EntryPoints(EntryPoints) {}

Domain_t InterMonoTaintAnalysis::join(const Domain_t &Lhs,
                                      const Domain_t &Rhs) {
  cout << "InterMonoTaintAnalysis::join()\n";
  Domain_t Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  cout << "Result size: " << Result.size() << endl;
  return Result;
}

bool InterMonoTaintAnalysis::sqSubSetEqual(const Domain_t &Lhs,
                                           const Domain_t &Rhs) {
  cout << "InterMonoTaintAnalysis::sqSubSetEqual()\n";
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
  // return true;
}

Domain_t InterMonoTaintAnalysis::normalFlow(const Node_t Stmt,
                                            const Domain_t &In) {
  cout << "InterMonoTaintAnalysis::normalFlow()\n";
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(Stmt)) {
    Result.insert(Alloc);
  }
  cout << "Result size: " << Result.size() << endl;
  return Result;
}

Domain_t InterMonoTaintAnalysis::callFlow(const Node_t CallSite,
                                          const Method_t Callee,
                                          const Domain_t &In) {
  cout << "InterMonoTaintAnalysis::callFlow()\n";
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Call = llvm::dyn_cast<llvm::CallInst>(CallSite)) {
    Result.insert(Call);
  }
  cout << "Result size: " << Result.size() << endl;
  return Result;
}

Domain_t InterMonoTaintAnalysis::returnFlow(const Node_t CallSite,
                                            const Method_t Callee,
                                            const Node_t RetSite,
                                            const Domain_t &In) {
  cout << "InterMonoTaintAnalysis::returnFlow()\n";
  return In;
}

Domain_t InterMonoTaintAnalysis::callToRetFlow(const Node_t CallSite,
                                               const Node_t RetSite,
                                               const Domain_t &In) {
  cout << "InterMonoTaintAnalysis::callToRetFlow()\n";
  return In;
}

MonoMap<Node_t, Domain_t> InterMonoTaintAnalysis::initialSeeds() {
  cout << "InterMonoTaintAnalysis::initialSeeds()\n";
  const Method_t main = ICFG.getMethod("main");
  MonoMap<Node_t, Domain_t> Seeds;
  Seeds.insert(make_pair(&main->front().front(), Domain_t()));
  return Seeds;
}

void InterMonoTaintAnalysis::printNode(ostream &os, Node_t n) const {
  os << llvmIRToString(n);
}

void InterMonoTaintAnalysis::printDataFlowFact(ostream &os, Domain_t d) const {
  for (auto fact : d) {
    os << llvmIRToString(fact) << '\n';
  }
}

void InterMonoTaintAnalysis::printMethod(ostream &os, Method_t m) const {
  os << m->getName().str();
}

bool InterMonoTaintAnalysis::recompute(const Method_t Callee) { return false; }

} // namespace psr
