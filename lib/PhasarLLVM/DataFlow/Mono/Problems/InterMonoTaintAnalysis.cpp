/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/Mono/Problems/InterMonoTaintAnalysis.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigUtilities.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include <unordered_map>
#include <utility>

namespace psr {

InterMonoTaintAnalysis::InterMonoTaintAnalysis(
    const LLVMProjectIRDB *IRDB, const DIBasedTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMAliasInfoRef PT,
    const LLVMTaintConfig &Config, std::vector<std::string> EntryPoints)
    : InterMonoProblem<InterMonoTaintAnalysisDomain>(IRDB, TH, ICF, PT,
                                                     std::move(EntryPoints)),
      Config(Config) {}

InterMonoTaintAnalysis::mono_container_t InterMonoTaintAnalysis::merge(
    const InterMonoTaintAnalysis::mono_container_t &Lhs,
    const InterMonoTaintAnalysis::mono_container_t &Rhs) {
  PHASAR_LOG_LEVEL(DEBUG, "InterMonoTaintAnalysis::join()");
  // cout << "InterMonoTaintAnalysis::join()\n";
  return Lhs.setUnion(Rhs);
}

bool InterMonoTaintAnalysis::equal_to(
    const InterMonoTaintAnalysis::mono_container_t &Lhs,
    const InterMonoTaintAnalysis::mono_container_t &Rhs) {
  PHASAR_LOG_LEVEL(DEBUG, "InterMonoTaintAnalysis::sqSubSetEqual()");
  return Rhs == Lhs;
}

InterMonoTaintAnalysis::mono_container_t InterMonoTaintAnalysis::normalFlow(
    InterMonoTaintAnalysis::n_t Inst,
    const InterMonoTaintAnalysis::mono_container_t &In) {
  PHASAR_LOG_LEVEL(DEBUG, "InterMonoTaintAnalysis::normalFlow()");
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
  PHASAR_LOG_LEVEL(DEBUG, "InterMonoTaintAnalysis::callFlow()");
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
  PHASAR_LOG_LEVEL(DEBUG, "InterMonoTaintAnalysis::returnFlow()");
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
    InterMonoTaintAnalysis::n_t /*RetSite*/, llvm::ArrayRef<f_t> Callees,
    const InterMonoTaintAnalysis::mono_container_t &In) {
  PHASAR_LOG_LEVEL(DEBUG, "InterMonoTaintAnalysis::callToRetFlow()");
  InterMonoTaintAnalysis::mono_container_t Out(In);
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  mono_container_t Gen;
  mono_container_t Kill;
  bool First = true;
  //-----------------------------------------------------------------------------
  // Handle virtual calls in the loop
  //-----------------------------------------------------------------------------
  for (const auto *Callee : Callees) {
    PHASAR_LOG_LEVEL(DEBUG, "InterMonoTaintAnalysis::callToRetFlow()::"
                                << Callee->getName());

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
    for (unsigned Idx = 0; Idx < CS->arg_size(); ++Idx) {
      if (CS->getArgOperand(Idx)->getType()->isPointerTy()) {
        Out.erase(CS->getArgOperand(Idx));
      }
    }
  }

  return Out;
}

std::unordered_map<InterMonoTaintAnalysis::n_t,
                   InterMonoTaintAnalysis::mono_container_t>
InterMonoTaintAnalysis::initialSeeds() {
  PHASAR_LOG_LEVEL(DEBUG, "InterMonoTaintAnalysis::initialSeeds()");
  const llvm::Function *Main = ICF->getFunction("main");
  std::unordered_map<InterMonoTaintAnalysis::n_t,
                     InterMonoTaintAnalysis::mono_container_t>
      Seeds;
  InterMonoTaintAnalysis::mono_container_t Facts;
  for (unsigned Idx = 0; Idx < Main->arg_size(); ++Idx) {
    Facts.insert(getNthFunctionArgument(Main, Idx));
  }
  Seeds.insert({&Main->front().front(), Facts});
  return Seeds;
}

const std::map<InterMonoTaintAnalysis::n_t,
               std::set<InterMonoTaintAnalysis::d_t>> &
InterMonoTaintAnalysis::getAllLeaks() const {
  return Leaks;
}

} // namespace psr
