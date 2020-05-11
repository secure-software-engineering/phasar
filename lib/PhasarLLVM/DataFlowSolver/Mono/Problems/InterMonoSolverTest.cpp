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
#include <utility>

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoSolverTest.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;

namespace psr {

InterMonoSolverTest::InterMonoSolverTest(const ProjectIRDB *IRDB,
                                         const LLVMTypeHierarchy *TH,
                                         const LLVMBasedICFG *ICF,
                                         const LLVMPointsToInfo *PT,
                                         std::set<std::string> EntryPoints)
    : InterMonoProblem<InterMonoSolverTest::n_t, InterMonoSolverTest::d_t,
                       InterMonoSolverTest::f_t, InterMonoSolverTest::t_t,
                       InterMonoSolverTest::v_t, InterMonoSolverTest::i_t,
                       InterMonoSolverTest::container_t>(
          IRDB, TH, ICF, PT, std::move(EntryPoints)) {}

InterMonoSolverTest::container_t
InterMonoSolverTest::merge(const InterMonoSolverTest::container_t &Lhs,
                           const InterMonoSolverTest::container_t &Rhs) {
  cout << "InterMonoSolverTest::join()\n";
  return Lhs.setUnion(Rhs);
}

bool InterMonoSolverTest::equal_to(
    const InterMonoSolverTest::container_t &Lhs,
    const InterMonoSolverTest::container_t &Rhs) {
  cout << "InterMonoSolverTest::equal_to()\n";
  return Lhs == Rhs;
}

InterMonoSolverTest::container_t
InterMonoSolverTest::normalFlow(const llvm::Instruction *Stmt,
                                const InterMonoSolverTest::container_t &In) {
  cout << "InterMonoSolverTest::normalFlow()\n";
  InterMonoSolverTest::container_t Result;
  Result = Result.setUnion(In);
  if (const auto *const Alloc = llvm::dyn_cast<llvm::AllocaInst>(Stmt)) {
    Result.insert(Alloc);
  }
  return In;
}

InterMonoSolverTest::container_t
InterMonoSolverTest::callFlow(const llvm::Instruction *CallSite,
                              const llvm::Function *Callee,
                              const InterMonoSolverTest::container_t &In) {
  cout << "InterMonoSolverTest::callFlow()\n";
  InterMonoSolverTest::container_t Result;
  Result.setUnion(In);
  if (const auto *const Call = llvm::dyn_cast<llvm::CallInst>(CallSite)) {
    Result.insert(Call);
  }
  return In;
}

InterMonoSolverTest::container_t InterMonoSolverTest::returnFlow(
    const llvm::Instruction *CallSite, const llvm::Function *Callee,
    const llvm::Instruction *ExitStmt, const llvm::Instruction *RetSite,
    const InterMonoSolverTest::container_t &In) {
  cout << "InterMonoSolverTest::returnFlow()\n";
  return In;
}

InterMonoSolverTest::container_t
InterMonoSolverTest::callToRetFlow(const llvm::Instruction *CallSite,
                                   const llvm::Instruction *RetSite,
                                   set<const llvm::Function *> Callees,
                                   const InterMonoSolverTest::container_t &In) {
  cout << "InterMonoSolverTest::callToRetFlow()\n";
  return In;
}

unordered_map<const llvm::Instruction *, InterMonoSolverTest::container_t>
InterMonoSolverTest::initialSeeds() {
  cout << "InterMonoSolverTest::initialSeeds()\n";
  unordered_map<const llvm::Instruction *, InterMonoSolverTest::container_t>
      Seeds;
  const llvm::Function *Main = ICF->getFunction("main");
  for (const auto *StartPoint : ICF->getStartPointsOf(Main)) {
    Seeds.insert(make_pair(StartPoint, allTop()));
  }
  return Seeds;
}

void InterMonoSolverTest::printNode(ostream &OS,
                                    const llvm::Instruction *N) const {
  OS << llvmIRToString(N);
}

void InterMonoSolverTest::printDataFlowFact(ostream &OS,
                                            const llvm::Value *D) const {
  OS << llvmIRToString(D) << '\n';
}

void InterMonoSolverTest::printFunction(ostream &OS,
                                        const llvm::Function *M) const {
  OS << M->getName().str();
}

} // namespace psr
