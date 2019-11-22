/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>
#include <set>

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

BitVectorSet<const llvm::Value *>
InterMonoSolverTest::join(const BitVectorSet<const llvm::Value *> &Lhs,
                          const BitVectorSet<const llvm::Value *> &Rhs) {
  cout << "InterMonoSolverTest::join()\n";
  return Lhs.setUnion(Rhs);
}

bool InterMonoSolverTest::sqSubSetEqual(
    const BitVectorSet<const llvm::Value *> &Lhs,
    const BitVectorSet<const llvm::Value *> &Rhs) {
  cout << "InterMonoSolverTest::sqSubSetEqual()\n";
  return Lhs.includes(Rhs);
}

BitVectorSet<const llvm::Value *>
InterMonoSolverTest::normalFlow(const llvm::Instruction *Stmt,
                                const BitVectorSet<const llvm::Value *> &In) {
  cout << "InterMonoSolverTest::normalFlow()\n";
  BitVectorSet<const llvm::Value *> Result;
  Result = Result.setUnion(In);
  if (const auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(Stmt)) {
    Result.insert(Alloc);
  }
  return In;
}

BitVectorSet<const llvm::Value *>
InterMonoSolverTest::callFlow(const llvm::Instruction *CallSite,
                              const llvm::Function *Callee,
                              const BitVectorSet<const llvm::Value *> &In) {
  cout << "InterMonoSolverTest::callFlow()\n";
  BitVectorSet<const llvm::Value *> Result;
  Result.setUnion(In);
  if (const auto Call = llvm::dyn_cast<llvm::CallInst>(CallSite)) {
    Result.insert(Call);
  }
  return In;
}

BitVectorSet<const llvm::Value *> InterMonoSolverTest::returnFlow(
    const llvm::Instruction *CallSite, const llvm::Function *Callee,
    const llvm::Instruction *ExitStmt, const llvm::Instruction *RetSite,
    const BitVectorSet<const llvm::Value *> &In) {
  cout << "InterMonoSolverTest::returnFlow()\n";
  return In;
}

BitVectorSet<const llvm::Value *> InterMonoSolverTest::callToRetFlow(
    const llvm::Instruction *CallSite, const llvm::Instruction *RetSite,
    set<const llvm::Function *> Callees,
    const BitVectorSet<const llvm::Value *> &In) {
  cout << "InterMonoSolverTest::callToRetFlow()\n";
  return In;
}

unordered_map<const llvm::Instruction *, BitVectorSet<const llvm::Value *>>
InterMonoSolverTest::initialSeeds() {
  cout << "InterMonoSolverTest::initialSeeds()\n";
  const llvm::Function *main = ICFG.getMethod("main");
  unordered_map<const llvm::Instruction *, BitVectorSet<const llvm::Value *>>
      Seeds;
  Seeds.insert(
      make_pair(&main->front().front(), BitVectorSet<const llvm::Value *>()));
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
