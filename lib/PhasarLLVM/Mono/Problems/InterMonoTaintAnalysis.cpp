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

InterMonoTaintAnalysis::InterMonoTaintAnalysis(LLVMBasedICFG &Icfg,
                                               vector<string> EntryPoints)
    : InterMonoProblem<const llvm::Instruction *, const llvm::Value *,
                       const llvm::Function *, LLVMBasedICFG &>(Icfg),
      EntryPoints(EntryPoints) {}

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::join(const MonoSet<const llvm::Value *> &Lhs,
                             const MonoSet<const llvm::Value *> &Rhs) {
  cout << "InterMonoTaintAnalysis::join()\n";
  MonoSet<const llvm::Value *> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  cout << "Result size: " << Result.size() << endl;
  return Result;
}

bool InterMonoTaintAnalysis::sqSubSetEqual(
    const MonoSet<const llvm::Value *> &Lhs,
    const MonoSet<const llvm::Value *> &Rhs) {
  cout << "InterMonoTaintAnalysis::sqSubSetEqual()\n";
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::normalFlow(const llvm::Instruction *Stmt,
                                   const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonoTaintAnalysis::normalFlow()\n";
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(Stmt)) {
    Result.insert(Alloc);
  }
  cout << "Result size: " << Result.size() << endl;
  return Result;
}

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::callFlow(const llvm::Instruction *CallSite,
                                 const llvm::Function *Callee,
                                 const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonoTaintAnalysis::callFlow()\n";
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Call = llvm::dyn_cast<llvm::CallInst>(CallSite)) {
    Result.insert(Call);
  }
  cout << "Result size: " << Result.size() << endl;
  return Result;
}

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::returnFlow(const llvm::Instruction *CallSite,
                                   const llvm::Function *Callee,
                                   const llvm::Instruction *RetSite,
                                   const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonoTaintAnalysis::returnFlow()\n";
  return In;
}

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::callToRetFlow(const llvm::Instruction *CallSite,
                                      const llvm::Instruction *RetSite,
                                      const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonoTaintAnalysis::callToRetFlow()\n";
  return In;
}

MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
InterMonoTaintAnalysis::initialSeeds() {
  cout << "InterMonoTaintAnalysis::initialSeeds()\n";
  const llvm::Function *main = ICFG.getMethod("main");
  MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>> Seeds;
  Seeds.insert(
      make_pair(&main->front().front(), MonoSet<const llvm::Value *>()));
  return Seeds;
}

void InterMonoTaintAnalysis::printNode(ostream &os,
                                       const llvm::Instruction *n) const {
  os << llvmIRToString(n);
}

void InterMonoTaintAnalysis::printDataFlowFact(ostream &os,
                                               const llvm::Value *d) const {
  os << llvmIRToString(d) << '\n';
}

void InterMonoTaintAnalysis::printMethod(ostream &os,
                                         const llvm::Function *m) const {
  os << m->getName().str();
}

} // namespace psr
