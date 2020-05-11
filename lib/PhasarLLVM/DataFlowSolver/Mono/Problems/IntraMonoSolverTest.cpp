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
#include <utility>

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoSolverTest.h"

using namespace std;
using namespace psr;

namespace psr {

IntraMonoSolverTest::IntraMonoSolverTest(const ProjectIRDB *IRDB,
                                         const LLVMTypeHierarchy *TH,
                                         const LLVMBasedCFG *CF,
                                         const LLVMPointsToInfo *PT,
                                         std::set<std::string> EntryPoints)
    : IntraMonoProblem<IntraMonoSolverTest::n_t, IntraMonoSolverTest::d_t,
                       IntraMonoSolverTest::f_t, IntraMonoSolverTest::t_t,
                       IntraMonoSolverTest::v_t, IntraMonoSolverTest::i_t,
                       IntraMonoSolverTest::container_t>(
          IRDB, TH, CF, PT, std::move(EntryPoints)) {}

IntraMonoSolverTest::container_t
IntraMonoSolverTest::merge(const IntraMonoSolverTest::container_t &Lhs,
                           const IntraMonoSolverTest::container_t &Rhs) {
  cout << "IntraMonoSolverTest::merge()\n";
  return Lhs.setUnion(Rhs);
}

bool IntraMonoSolverTest::equal_to(
    const IntraMonoSolverTest::container_t &Lhs,
    const IntraMonoSolverTest::container_t &Rhs) {
  cout << "IntraMonoSolverTest::equal_to()\n";
  return Lhs == Rhs;
}

IntraMonoSolverTest::container_t
IntraMonoSolverTest::normalFlow(const llvm::Instruction *Stmt,
                                const IntraMonoSolverTest::container_t &In) {
  cout << "IntraMonoSolverTest::normalFlow()\n";
  IntraMonoSolverTest::container_t Result = In;
  if (const auto *const Store = llvm::dyn_cast<llvm::StoreInst>(Stmt)) {
    Result.insert(Store);
  }
  return Result;
}

unordered_map<const llvm::Instruction *, IntraMonoSolverTest::container_t>
IntraMonoSolverTest::initialSeeds() {
  cout << "IntraMonoSolverTest::initialSeeds()\n";
  return {};
}

void IntraMonoSolverTest::printNode(ostream &OS,
                                    const llvm::Instruction *Stmt) const {
  OS << llvmIRToString(Stmt);
}

void IntraMonoSolverTest::printDataFlowFact(ostream &OS,
                                            const llvm::Value *FlowFact) const {
  OS << llvmIRToString(FlowFact);
}

void IntraMonoSolverTest::printFunction(ostream &OS,
                                        const llvm::Function *Fun) const {
  OS << Fun->getName().str();
}

} // namespace psr
