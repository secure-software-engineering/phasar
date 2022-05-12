/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

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

namespace psr {

InterMonoSolverTest::InterMonoSolverTest(const ProjectIRDB *IRDB,
                                         const LLVMTypeHierarchy *TH,
                                         const LLVMBasedICFG *ICF,
                                         const LLVMPointsToInfo *PT,
                                         std::set<std::string> EntryPoints)
    : InterMonoProblem<InterMonoSolverTestDomain>(IRDB, TH, ICF, PT,
                                                  std::move(EntryPoints)) {}

InterMonoSolverTest::mono_container_t
InterMonoSolverTest::merge(const InterMonoSolverTest::mono_container_t &Lhs,
                           const InterMonoSolverTest::mono_container_t &Rhs) {
  llvm::outs() << "InterMonoSolverTest::join()\n";
  return Lhs.setUnion(Rhs);
}

bool InterMonoSolverTest::equal_to(
    const InterMonoSolverTest::mono_container_t &Lhs,
    const InterMonoSolverTest::mono_container_t &Rhs) {
  llvm::outs() << "InterMonoSolverTest::equal_to()\n";
  return Lhs == Rhs;
}

InterMonoSolverTest::mono_container_t InterMonoSolverTest::normalFlow(
    InterMonoSolverTest::n_t Inst,
    const InterMonoSolverTest::mono_container_t &In) {
  llvm::outs() << "InterMonoSolverTest::normalFlow()\n";
  InterMonoSolverTest::mono_container_t Result;
  Result = Result.setUnion(In);
  if (const auto *const Alloc = llvm::dyn_cast<llvm::AllocaInst>(Inst)) {
    Result.insert(Alloc);
  }
  return In;
}

InterMonoSolverTest::mono_container_t
InterMonoSolverTest::callFlow(InterMonoSolverTest::n_t CallSite,
                              InterMonoSolverTest::f_t /*Callee*/,
                              const InterMonoSolverTest::mono_container_t &In) {
  llvm::outs() << "InterMonoSolverTest::callFlow()\n";
  InterMonoSolverTest::mono_container_t Result;
  Result = Result.setUnion(In);
  if (const auto *const Call = llvm::dyn_cast<llvm::CallInst>(CallSite)) {
    Result.insert(Call);
  }
  return In;
}

InterMonoSolverTest::mono_container_t InterMonoSolverTest::returnFlow(
    InterMonoSolverTest::n_t /*CallSite*/, InterMonoSolverTest::f_t /*Callee*/,
    InterMonoSolverTest::n_t /*ExitStmt*/, InterMonoSolverTest::n_t /*RetSite*/,
    const InterMonoSolverTest::mono_container_t &In) {
  llvm::outs() << "InterMonoSolverTest::returnFlow()\n";
  return In;
}

InterMonoSolverTest::mono_container_t InterMonoSolverTest::callToRetFlow(
    InterMonoSolverTest::n_t /*CallSite*/, InterMonoSolverTest::n_t /*RetSite*/,
    std::set<InterMonoSolverTest::f_t> /*Callees*/,
    const InterMonoSolverTest::mono_container_t &In) {
  llvm::outs() << "InterMonoSolverTest::callToRetFlow()\n";
  return In;
}

std::unordered_map<InterMonoSolverTest::n_t,
                   InterMonoSolverTest::mono_container_t>
InterMonoSolverTest::initialSeeds() {
  llvm::outs() << "InterMonoSolverTest::initialSeeds()\n";
  std::unordered_map<InterMonoSolverTest::n_t,
                     InterMonoSolverTest::mono_container_t>
      Seeds;
  InterMonoSolverTest::f_t Main = ICF->getFunction("main");
  for (const auto *StartPoint : ICF->getStartPointsOf(Main)) {
    Seeds.insert({StartPoint, allTop()});
  }
  return Seeds;
}

void InterMonoSolverTest::printNode(llvm::raw_ostream &OS,
                                    InterMonoSolverTest::n_t Inst) const {
  OS << llvmIRToString(Inst);
}

void InterMonoSolverTest::printDataFlowFact(
    llvm::raw_ostream &OS, InterMonoSolverTest::d_t Fact) const {
  OS << llvmIRToString(Fact) << '\n';
}

void InterMonoSolverTest::printFunction(llvm::raw_ostream &OS,
                                        InterMonoSolverTest::f_t Fun) const {
  OS << Fun->getName();
}

} // namespace psr
