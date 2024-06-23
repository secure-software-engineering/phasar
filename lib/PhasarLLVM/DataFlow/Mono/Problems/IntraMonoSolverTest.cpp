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

#include "phasar/PhasarLLVM/DataFlow/Mono/Problems/IntraMonoSolverTest.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include <algorithm>
#include <utility>

using namespace psr;

namespace psr {

IntraMonoSolverTest::IntraMonoSolverTest(const LLVMProjectIRDB *IRDB,
                                         const DIBasedTypeHierarchy *TH,
                                         const LLVMBasedCFG *CF,
                                         LLVMAliasInfoRef PT,
                                         std::vector<std::string> EntryPoints)
    : IntraMonoProblem<IntraMonoSolverTestAnalysisDomain>(
          IRDB, TH, CF, PT, std::move(EntryPoints)) {}

IntraMonoSolverTest::mono_container_t
IntraMonoSolverTest::merge(const IntraMonoSolverTest::mono_container_t &Lhs,
                           const IntraMonoSolverTest::mono_container_t &Rhs) {
  llvm::outs() << "IntraMonoSolverTest::merge()\n";
  return Lhs.setUnion(Rhs);
}

bool IntraMonoSolverTest::equal_to(
    const IntraMonoSolverTest::mono_container_t &Lhs,
    const IntraMonoSolverTest::mono_container_t &Rhs) {
  llvm::outs() << "IntraMonoSolverTest::equal_to()\n";
  return Lhs == Rhs;
}

IntraMonoSolverTest::mono_container_t IntraMonoSolverTest::normalFlow(
    IntraMonoSolverTest::n_t Inst,
    const IntraMonoSolverTest::mono_container_t &In) {
  llvm::outs() << "IntraMonoSolverTest::normalFlow()\n";
  IntraMonoSolverTest::mono_container_t Result = In;
  if (const auto *const Store = llvm::dyn_cast<llvm::StoreInst>(Inst)) {
    Result.insert(Store);
  }
  return Result;
}

std::unordered_map<IntraMonoSolverTest::n_t,
                   IntraMonoSolverTest::mono_container_t>
IntraMonoSolverTest::initialSeeds() {
  llvm::outs() << "IntraMonoSolverTest::initialSeeds()\n";
  return {};
}

} // namespace psr
