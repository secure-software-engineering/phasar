/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann and others
 *****************************************************************************/

#include <algorithm>
#include <unordered_map>
#include <utility>

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/BitVectorSet.h"

using namespace std;
using namespace psr;

namespace psr {

InterMonoFullConstantPropagation::InterMonoFullConstantPropagation(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints)
    : IntraMonoFullConstantPropagation(IRDB, TH, ICF, PT, EntryPoints),
      InterMonoProblem<IntraMonoFullConstantPropagationAnalysisDomain>(
          IRDB, TH, ICF, PT, EntryPoints) {}

InterMonoFullConstantPropagation::mono_container_t
InterMonoFullConstantPropagation::merge(
    const InterMonoFullConstantPropagation::mono_container_t & /*Lhs*/,
    const InterMonoFullConstantPropagation::mono_container_t & /*Rhs*/) {
  // TODO implement
  return {};
}

bool InterMonoFullConstantPropagation::equal_to(
    const InterMonoFullConstantPropagation::mono_container_t &Lhs,
    const InterMonoFullConstantPropagation::mono_container_t &Rhs) {
  return IntraMonoFullConstantPropagation::equal_to(Lhs, Rhs);
}

std::unordered_map<InterMonoFullConstantPropagation::n_t,
                   InterMonoFullConstantPropagation::mono_container_t>
InterMonoFullConstantPropagation::initialSeeds() {
  return IntraMonoFullConstantPropagation::initialSeeds();
}

InterMonoFullConstantPropagation::mono_container_t
InterMonoFullConstantPropagation::normalFlow(
    InterMonoFullConstantPropagation::n_t Inst,
    const InterMonoFullConstantPropagation::mono_container_t &In) {
  return IntraMonoFullConstantPropagation::normalFlow(Inst, In);
}

InterMonoFullConstantPropagation::mono_container_t
InterMonoFullConstantPropagation::callFlow(
    InterMonoFullConstantPropagation::n_t CallSite,
    InterMonoFullConstantPropagation::f_t Callee,
    const InterMonoFullConstantPropagation::mono_container_t &In) {
  InterMonoFullConstantPropagation::mono_container_t Out;

  // Map the actual parameters into the formal parameters
  if (llvm::isa<llvm::CallInst>(CallSite) ||
      llvm::isa<llvm::InvokeInst>(CallSite)) {
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
    // early exit; varargs not handled yet
    if (CS->arg_empty() || Callee->isVarArg()) {
      return Out;
    }
    vector<const llvm::Value *> Actuals;
    vector<const llvm::Value *> Formals;
    // Set up the actual parameters
    for (unsigned Idx = 0; Idx < CS->arg_size(); ++Idx) {
      Actuals.push_back(CS->getArgOperand(Idx));
    }
    // Set up the formal parameters
    for (unsigned Idx = 0; Idx < Callee->arg_size(); ++Idx) {
      Formals.push_back(Callee->getArg(Idx));
    }
    // Perform mapping
    for (unsigned Idx = 0; Idx < Actuals.size(); ++Idx) {
      auto Search = In.find(Actuals[Idx]);
      if (Search != In.end()) {
        Out.insert({Formals[Idx], Search->second});
      }
      // check for integer literals
      if (const auto *ConstInt =
              llvm::dyn_cast<llvm::ConstantInt>(Actuals[Idx])) {
        llvm::outs() << "Found literal!\n";
        Out.insert({Formals[Idx], ConstInt->getSExtValue()});
      }
    }
  }
  // TODO: Handle globals
  /*
  if (llvm::isa<llvm::GlobalVariable>(source)) {
    res.insert(source);
  }*/
  return Out;
}

InterMonoFullConstantPropagation::mono_container_t
InterMonoFullConstantPropagation::returnFlow(
    InterMonoFullConstantPropagation::n_t CallSite,
    InterMonoFullConstantPropagation::f_t /*Callee*/,
    InterMonoFullConstantPropagation::n_t ExitStmt,
    InterMonoFullConstantPropagation::n_t /*RetSite*/,
    const InterMonoFullConstantPropagation::mono_container_t &In) {
  InterMonoFullConstantPropagation::mono_container_t Out;

  if (const auto *Return = llvm::dyn_cast<llvm::ReturnInst>(ExitStmt)) {
    if (Return->getReturnValue()->getType()->isIntegerTy()) {
      // Return value is integer literal
      if (auto *ConstInt =
              llvm::dyn_cast<llvm::ConstantInt>(Return->getReturnValue())) {
        Out.insert({CallSite, ConstInt->getSExtValue()});
      } else {
        // handle return of integer variable
        auto Search = In.find(Return->getReturnValue());
        if (Search != In.end()) {
          llvm::outs() << "Found const return variable\n";
          Out.insert({CallSite, Search->second});
        }
      }
      // handle Global Variables
      // TODO:handle globals
    }
  }
  return Out;
}

InterMonoFullConstantPropagation::mono_container_t
InterMonoFullConstantPropagation::callToRetFlow(
    InterMonoFullConstantPropagation::n_t /*CallSite*/,
    InterMonoFullConstantPropagation::n_t /*RetSite*/,
    llvm::ArrayRef<f_t> /*Callees*/,
    const InterMonoFullConstantPropagation::mono_container_t &In) {
  return In;
}

void InterMonoFullConstantPropagation::printNode(
    llvm::raw_ostream &OS, InterMonoFullConstantPropagation::n_t Inst) const {
  IntraMonoFullConstantPropagation::printNode(OS, Inst);
}

void InterMonoFullConstantPropagation::printDataFlowFact(
    llvm::raw_ostream &OS, InterMonoFullConstantPropagation::d_t Fact) const {
  IntraMonoFullConstantPropagation::printDataFlowFact(OS, Fact);
}

void InterMonoFullConstantPropagation::printFunction(
    llvm::raw_ostream &OS, InterMonoFullConstantPropagation::f_t Fun) const {
  IntraMonoFullConstantPropagation::printFunction(OS, Fun);
}

void InterMonoFullConstantPropagation::printContainer(
    llvm::raw_ostream &OS,
    InterMonoFullConstantPropagation::mono_container_t Con) const {
  for (const auto &[Var, Val] : Con) {
    OS << "<" << llvmIRToString(Var) << ", " << Val << ">, ";
  }
}

} // namespace psr
