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

InterMonoSolverTest::InterMonoSolverTest(LLVMBasedICFG &Icfg,
                                         vector<string> EntryPoints)
    : InterMonoProblem<const llvm::Instruction *, const llvm::Value *,
                       const llvm::Function *, LLVMBasedICFG &>(Icfg),
      EntryPoints(EntryPoints) {}

MonoSet<const llvm::Value *>
InterMonoSolverTest::join(const MonoSet<const llvm::Value *> &Lhs,
                          const MonoSet<const llvm::Value *> &Rhs) {
  cout << "InterMonoSolverTest::join()\n";
  MonoSet<const llvm::Value *> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool InterMonoSolverTest::sqSubSetEqual(
    const MonoSet<const llvm::Value *> &Lhs,
    const MonoSet<const llvm::Value *> &Rhs) {
  cout << "InterMonoSolverTest::sqSubSetEqual()\n";
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<const llvm::Value *>
InterMonoSolverTest::normalFlow(const llvm::Instruction *Stmt,
                                const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonoSolverTest::normalFlow()\n";
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(Stmt)) {
    Result.insert(Alloc);
  }
  return In;
}

MonoSet<const llvm::Value *>
InterMonoSolverTest::callFlow(const llvm::Instruction *CallSite,
                              const llvm::Function *Callee,
                              const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonoSolverTest::callFlow()\n";
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Call = llvm::dyn_cast<llvm::CallInst>(CallSite)) {
    Result.insert(Call);
  }
  return In;
}

MonoSet<const llvm::Value *> InterMonoSolverTest::returnFlow(
    const llvm::Instruction *CallSite, const llvm::Function *Callee,
    const llvm::Instruction *RetSite, const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonoSolverTest::returnFlow()\n";
  return In;
}

MonoSet<const llvm::Value *>
InterMonoSolverTest::callToRetFlow(const llvm::Instruction *CallSite,
                                   const llvm::Instruction *RetSite,
                                   const MonoSet<const llvm::Value *> &In) {
  cout << "InterMonoSolverTest::callToRetFlow()\n";
  return In;
}

MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
InterMonoSolverTest::initialSeeds() {
  cout << "InterMonoSolverTest::initialSeeds()\n";
  const llvm::Function *main = ICFG.getMethod("main");
  MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>> Seeds;
  Seeds.insert(
      make_pair(&main->front().front(), MonoSet<const llvm::Value *>()));
  return Seeds;
}

void InterMonoSolverTest::printNode(ostream &os,
                                    const llvm::Instruction *n) const {
  os << llvmIRToString(n);
}

void InterMonoSolverTest::printDataFlowFact(ostream &os,
                                            const llvm::Value *d) const {
  os << llvmIRToString(d) << '\n';
}

void InterMonoSolverTest::printMethod(ostream &os,
                                      const llvm::Function *m) const {
  os << m->getName().str();
}

} // namespace psr
