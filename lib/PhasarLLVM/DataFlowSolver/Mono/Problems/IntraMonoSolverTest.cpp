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

#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoSolverTest.h>
using namespace std;
using namespace psr;

namespace psr {

IntraMonoSolverTest::IntraMonoSolverTest(
    const ProjectIRDB *IRDB, const TypeHierarchy *TH, const LLVMBasedCFG *CF,
    const PointsToInfo *PT, std::initializer_list<std::string> EntryPoints)
    : IntraMonoProblem<const llvm::Instruction *, const llvm::Value *,
                       const llvm::Function *, LLVMBasedCFG>(IRDB, TH, CF, PT,
                                                               EntryPoints) {}

MonoSet<const llvm::Value *>
IntraMonoSolverTest::join(const MonoSet<const llvm::Value *> &Lhs,
                          const MonoSet<const llvm::Value *> &Rhs) {
  cout << "IntraMonoSolverTest::join()\n";
  MonoSet<const llvm::Value *> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool IntraMonoSolverTest::sqSubSetEqual(
    const MonoSet<const llvm::Value *> &Lhs,
    const MonoSet<const llvm::Value *> &Rhs) {
  cout << "IntraMonoSolverTest::sqSubSetEqual()\n";
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<const llvm::Value *>
IntraMonoSolverTest::normalFlow(const llvm::Instruction *S,
                                const MonoSet<const llvm::Value *> &In) {
  cout << "IntraMonoSolverTest::normalFlow()\n";
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Store = llvm::dyn_cast<llvm::StoreInst>(S)) {
    Result.insert(Store);
  }
  return Result;
}

MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
IntraMonoSolverTest::initialSeeds() {
  cout << "IntraMonoSolverTest::initialSeeds()\n";
  return {};
}

void IntraMonoSolverTest::printNode(ostream &os,
                                    const llvm::Instruction *n) const {
  os << llvmIRToString(n);
}

void IntraMonoSolverTest::printDataFlowFact(ostream &os,
                                            const llvm::Value *d) const {
  os << llvmIRToString(d);
}

void IntraMonoSolverTest::printMethod(ostream &os,
                                      const llvm::Function *m) const {
  os << m->getName().str();
}

} // namespace psr
