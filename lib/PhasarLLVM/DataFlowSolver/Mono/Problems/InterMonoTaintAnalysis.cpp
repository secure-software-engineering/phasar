/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>
#include <unordered_map>
#include <utility>

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoTaintAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigUtilities.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;

namespace psr {

InterMonoTaintAnalysis::InterMonoTaintAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    const TaintConfig &Config, std::set<std::string> EntryPoints)
    : InterMonoProblem<InterMonoTaintAnalysisDomain>(IRDB, TH, ICF, PT,
                                                     std::move(EntryPoints)),
      Config(Config) {}

InterMonoTaintAnalysis::mono_container_t InterMonoTaintAnalysis::merge(
    const InterMonoTaintAnalysis::mono_container_t &Lhs,
    const InterMonoTaintAnalysis::mono_container_t &Rhs) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "InterMonoTaintAnalysis::join()");
  // cout << "InterMonoTaintAnalysis::join()\n";
  return Lhs.setUnion(Rhs);
}

bool InterMonoTaintAnalysis::equal_to(
    const InterMonoTaintAnalysis::mono_container_t &Lhs,
    const InterMonoTaintAnalysis::mono_container_t &Rhs) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "InterMonoTaintAnalysis::sqSubSetEqual()");
  return Rhs == Lhs;
}

InterMonoTaintAnalysis::mono_container_t InterMonoTaintAnalysis::normalFlow(
    InterMonoTaintAnalysis::n_t Inst,
    const InterMonoTaintAnalysis::mono_container_t &In) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "InterMonoTaintAnalysis::normalFlow()");
  InterMonoTaintAnalysis::mono_container_t Out(In);
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Inst)) {
    if (In.count(Store->getValueOperand())) {
      Out.insert(Store->getPointerOperand());
    } else if (In.count(Store->getPointerOperand())) {
      Out.erase(Store->getPointerOperand());
    }
  }
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Inst)) {
    if (In.count(Load->getPointerOperand())) {
      Out.insert(Load);
    }
  }
  if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(Inst)) {
    if (In.count(Gep->getPointerOperand())) {
      Out.insert(Gep);
    }
  }
  if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(Inst)) {
    if (In.count(Cast->getOperand(0))) {
      Out.insert(Cast);
    }
  }
  return Out;
}

InterMonoTaintAnalysis::mono_container_t InterMonoTaintAnalysis::callFlow(
    InterMonoTaintAnalysis::n_t CallSite, const llvm::Function *Callee,
    const InterMonoTaintAnalysis::mono_container_t &In) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "InterMonoTaintAnalysis::callFlow()");
  InterMonoTaintAnalysis::mono_container_t Out;
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  for (unsigned Idx = 0; Idx < Callee->arg_size(); ++Idx) {
    if (In.count(CS->getArgOperand(Idx))) {
      Out.insert(Callee->getArg(Idx));
    }
  }
  return Out;
}

InterMonoTaintAnalysis::mono_container_t InterMonoTaintAnalysis::returnFlow(
    InterMonoTaintAnalysis::n_t CallSite, const llvm::Function *Callee,
    InterMonoTaintAnalysis::n_t ExitStmt,
    InterMonoTaintAnalysis::n_t /*RetSite*/,
    const InterMonoTaintAnalysis::mono_container_t &In) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "InterMonoTaintAnalysis::returnFlow()");
  InterMonoTaintAnalysis::mono_container_t Out;
  if (const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(ExitStmt)) {
    if (In.count(Ret->getReturnValue())) {
      Out.insert(CallSite);
    }
  }
  // propagate pointer arguments to the caller, since this callee may modify
  // them
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  unsigned Index = 0;
  for (const auto &Arg : Callee->args()) {
    if (Arg.getType()->isPointerTy() && In.count(&Arg)) {
      Out.insert(CS->getArgOperand(Index));
    }
    Index++;
  }
  return Out;
}

InterMonoTaintAnalysis::mono_container_t InterMonoTaintAnalysis::callToRetFlow(
    InterMonoTaintAnalysis::n_t CallSite,
    InterMonoTaintAnalysis::n_t /*RetSite*/,
    set<const llvm::Function *> Callees,
    const InterMonoTaintAnalysis::mono_container_t &In) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "InterMonoTaintAnalysis::callToRetFlow()");
  InterMonoTaintAnalysis::mono_container_t Out(In);
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  mono_container_t Gen;
  mono_container_t Kill;
  bool First = true;
  //-----------------------------------------------------------------------------
  // Handle virtual calls in the loop
  //-----------------------------------------------------------------------------
  for (const auto *Callee : Callees) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "InterMonoTaintAnalysis::callToRetFlow()::"
                  << Callee->getName().str());

    collectGeneratedFacts(Gen, Config, CS, Callee);
    collectLeakedFacts(Leaks[CS], Config, CS, Callee,
                       [&In](const llvm::Value *V) { return In.count(V); });
    if (First) {
      First = false;
      collectSanitizedFacts(Kill, Config, CS, Callee);
    } else {
      mono_container_t Tmp;
      collectSanitizedFacts(Tmp, Config, CS, Callee);
      intersectWith(Kill, Tmp);
    }
  }
  Out.erase(Kill);
  Out.insert(Gen);
  if (Gen.empty() && Kill.empty()) {
    // erase pointer arguments, since they are now propagated in the retFF
    for (unsigned Idx = 0; Idx < CS->getNumArgOperands(); ++Idx) {
      if (CS->getArgOperand(Idx)->getType()->isPointerTy()) {
        Out.erase(CS->getArgOperand(Idx));
      }
    }
  }

  return Out;
}

unordered_map<InterMonoTaintAnalysis::n_t,
              InterMonoTaintAnalysis::mono_container_t>
InterMonoTaintAnalysis::initialSeeds() {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "InterMonoTaintAnalysis::initialSeeds()");
  const llvm::Function *Main = ICF->getFunction("main");
  unordered_map<InterMonoTaintAnalysis::n_t,
                InterMonoTaintAnalysis::mono_container_t>
      Seeds;
  InterMonoTaintAnalysis::mono_container_t Facts;
  for (unsigned Idx = 0; Idx < Main->arg_size(); ++Idx) {
    Facts.insert(getNthFunctionArgument(Main, Idx));
  }
  Seeds.insert(make_pair(&Main->front().front(), Facts));
  return Seeds;
}

void InterMonoTaintAnalysis::printNode(ostream &OS,
                                       InterMonoTaintAnalysis::n_t Inst) const {
  OS << llvmIRToString(Inst);
}

void InterMonoTaintAnalysis::printDataFlowFact(
    ostream &OS, InterMonoTaintAnalysis::d_t Fact) const {
  OS << llvmIRToString(Fact) << '\n';
}

void InterMonoTaintAnalysis::printFunction(ostream &OS,
                                           const llvm::Function *Fun) const {
  OS << Fun->getName().str();
}
const std::map<InterMonoTaintAnalysis::n_t,
               std::set<InterMonoTaintAnalysis::d_t>> &
InterMonoTaintAnalysis::getAllLeaks() const {
  return Leaks;
}

} // namespace psr
