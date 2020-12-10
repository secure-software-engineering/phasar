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
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoSolverTest.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace std;
using namespace psr;

namespace psr {

IntraMonoSolverTest::IntraMonoSolverTest(const ProjectIRDB *IRDB,
                                         const LLVMTypeHierarchy *TH,
                                         const LLVMBasedCFG *CF,
                                         const LLVMPointsToInfo *PT,
                                         std::set<std::string> EntryPoints)
    : IntraMonoProblem<IntraMonoSolverTestAnalysisDomain>(
          IRDB, TH, CF, PT, std::move(EntryPoints)) {}

IntraMonoSolverTest::mono_container_t
IntraMonoSolverTest::merge(const IntraMonoSolverTest::mono_container_t &Lhs,
                           const IntraMonoSolverTest::mono_container_t &Rhs) {
  cout << "IntraMonoSolverTest::merge()\n";
  return Lhs.setUnion(Rhs);
}

bool IntraMonoSolverTest::equal_to(
    const IntraMonoSolverTest::mono_container_t &Lhs,
    const IntraMonoSolverTest::mono_container_t &Rhs) {
  cout << "IntraMonoSolverTest::equal_to()\n";
  return Lhs == Rhs;
}

IntraMonoSolverTest::mono_container_t IntraMonoSolverTest::normalFlow(
    IntraMonoSolverTest::n_t Inst,
    const IntraMonoSolverTest::mono_container_t &In) {
  cout << "IntraMonoSolverTest::normalFlow()\n";
  IntraMonoSolverTest::mono_container_t Result = In;
  if (const auto *const Store = llvm::dyn_cast<llvm::StoreInst>(Inst)) {
    Result.insert(Store);
  }
  return Result;
}

unordered_map<IntraMonoSolverTest::n_t, IntraMonoSolverTest::mono_container_t>
IntraMonoSolverTest::initialSeeds() {
  cout << "IntraMonoSolverTest::initialSeeds()\n";
  return {};
}

void IntraMonoSolverTest::printNode(ostream &OS,
                                    IntraMonoSolverTest::n_t Inst) const {
  OS << llvmIRToString(Inst);
}

void IntraMonoSolverTest::printDataFlowFact(
    ostream &OS, IntraMonoSolverTest::d_t Fact) const {
  OS << llvmIRToString(Fact);
}

void IntraMonoSolverTest::printFunction(ostream &OS,
                                        IntraMonoSolverTest::f_t Fun) const {
  OS << Fun->getName().str();
}

} // namespace psr
