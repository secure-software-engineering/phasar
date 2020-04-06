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

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoTaintAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/TaintConfiguration.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;

namespace psr {

InterMonoTaintAnalysis::InterMonoTaintAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    const TaintConfiguration<const llvm::Value *> &TSF,
    std::set<std::string> EntryPoints)
    : InterMonoProblem<InterMonoTaintAnalysis::n_t, InterMonoTaintAnalysis::d_t,
                       InterMonoTaintAnalysis::f_t, InterMonoTaintAnalysis::t_t,
                       InterMonoTaintAnalysis::v_t,
                       InterMonoTaintAnalysis::i_t>(IRDB, TH, ICF, PT,
                                                    EntryPoints),
      TSF(TSF) {}

BitVectorSet<const llvm::Value *>
InterMonoTaintAnalysis::join(const BitVectorSet<const llvm::Value *> &Lhs,
                             const BitVectorSet<const llvm::Value *> &Rhs) {
  auto &LG = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(LG, DEBUG) << "InterMonoTaintAnalysis::join()");
  // cout << "InterMonoTaintAnalysis::join()\n";
  return Lhs.setUnion(Rhs);
}

bool InterMonoTaintAnalysis::sqSubSetEqual(
    const BitVectorSet<const llvm::Value *> &Lhs,
    const BitVectorSet<const llvm::Value *> &Rhs) {
  auto &LG = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(LG, DEBUG)
                << "InterMonoTaintAnalysis::sqSubSetEqual()");
  return Rhs.includes(Lhs);
}

BitVectorSet<const llvm::Value *> InterMonoTaintAnalysis::normalFlow(
    const llvm::Instruction *Stmt,
    const BitVectorSet<const llvm::Value *> &In) {
  auto &LG = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(LG, DEBUG)
                << "InterMonoTaintAnalysis::normalFlow()");
  BitVectorSet<const llvm::Value *> Out(In);
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(Stmt)) {
    if (In.count(Store->getValueOperand())) {
      Out.insert(Store->getPointerOperand());
    } else if (In.count(Store->getPointerOperand())) {
      Out.erase(Store->getPointerOperand());
    }
  }
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(Stmt)) {
    if (In.count(Load->getPointerOperand())) {
      Out.insert(Load);
    }
  }
  if (auto Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(Stmt)) {
    if (In.count(Gep->getPointerOperand())) {
      Out.insert(Gep);
    }
  }
  return Out;
}

BitVectorSet<const llvm::Value *>
InterMonoTaintAnalysis::callFlow(const llvm::Instruction *CallSite,
                                 const llvm::Function *Callee,
                                 const BitVectorSet<const llvm::Value *> &In) {
  auto &LG = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(LG, DEBUG)
                << "InterMonoTaintAnalysis::callFlow()");
  BitVectorSet<const llvm::Value *> Out;
  llvm::ImmutableCallSite CS(CallSite);
  vector<const llvm::Value *> Actuals;
  vector<const llvm::Value *> Formals;
  // set up the actual parameters
  for (unsigned Idx = 0; Idx < CS.getNumArgOperands(); ++Idx) {
    Actuals.push_back(CS.getArgOperand(Idx));
  }
  // set up the formal parameters
  /* for (unsigned idx = 0; idx < Callee->arg_size(); ++idx) {
    Formals.push_back(getNthFunctionArgument(Callee, idx));
  }*/
  for (auto &Arg : Callee->args()) {
    Formals.push_back(&Arg);
  }
  for (unsigned Idx = 0; Idx < Actuals.size(); ++Idx) {
    if (In.count(Actuals[Idx])) {
      Out.insert(Formals[Idx]);
    }
  }
  return Out;
}

BitVectorSet<const llvm::Value *> InterMonoTaintAnalysis::returnFlow(
    const llvm::Instruction *CallSite, const llvm::Function *Callee,
    const llvm::Instruction *ExitStmt, const llvm::Instruction *RetSite,
    const BitVectorSet<const llvm::Value *> &In) {
  auto &LG = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(LG, DEBUG)
                << "InterMonoTaintAnalysis::returnFlow()");
  BitVectorSet<const llvm::Value *> Out;
  if (auto Ret = llvm::dyn_cast<llvm::ReturnInst>(ExitStmt)) {
    if (In.count(Ret->getReturnValue())) {
      Out.insert(CallSite);
    }
  }
  // propagate pointer arguments to the caller, since this callee may modify
  // them
  llvm::ImmutableCallSite CS(CallSite);
  unsigned Index = 0;
  for (auto &Arg : Callee->args()) {
    if (Arg.getType()->isPointerTy() && In.count(&Arg)) {
      Out.insert(CS.getArgOperand(Index));
    }
    Index++;
  }
  return Out;
}

BitVectorSet<const llvm::Value *> InterMonoTaintAnalysis::callToRetFlow(
    const llvm::Instruction *CallSite, const llvm::Instruction *RetSite,
    set<const llvm::Function *> Callees,
    const BitVectorSet<const llvm::Value *> &In) {
  auto &LG = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(LG, DEBUG)
                << "InterMonoTaintAnalysis::callToRetFlow()");
  BitVectorSet<const llvm::Value *> Out(In);
  llvm::ImmutableCallSite CS(CallSite);
  //-----------------------------------------------------------------------------
  // Handle virtual calls in the loop
  //-----------------------------------------------------------------------------
  for (auto Callee : Callees) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(LG, DEBUG)
                  << "InterMonoTaintAnalysis::callToRetFlow()::"
                  << Callee->getName().str());

    if (TSF.isSink(Callee->getName().str())) {
      for (unsigned Idx = 0; Idx < CS.getNumArgOperands(); ++Idx) {
        if (TSF.getSink(Callee->getName().str()).isLeakedArg(Idx) &&
            In.count(CS.getArgOperand(Idx))) {
          cout << "FOUND LEAK AT: " << llvmIRToString(CallSite) << '\n';
          cout << "LEAKED VALUE: " << llvmIRToString(CS.getArgOperand(Idx))
               << '\n'
               << endl;
          Leaks[CallSite].insert(CS.getArgOperand(Idx));
        }
      }
    }
    if (TSF.isSource(Callee->getName().str())) {
      for (unsigned Idx = 0; Idx < CS.getNumArgOperands(); ++Idx) {
        if (TSF.getSource(Callee->getName().str()).isTaintedArg(Idx)) {
          Out.insert(CS.getArgOperand(Idx));
        }
      }
      if (TSF.getSource(Callee->getName().str()).TaintsReturn) {
        Out.insert(CallSite);
      }
    }
  }

  // erase pointer arguments, since they are now propagated in the retFF
  for (unsigned Idx = 0; Idx < CS.getNumArgOperands(); ++Idx) {
    if (CS.getArgOperand(Idx)->getType()->isPointerTy()) {
      Out.erase(CS.getArgOperand(Idx));
    }
  }
  return Out;
}

unordered_map<const llvm::Instruction *, BitVectorSet<const llvm::Value *>>
InterMonoTaintAnalysis::initialSeeds() {
  auto &LG = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(LG, DEBUG)
                << "InterMonoTaintAnalysis::initialSeeds()");
  const llvm::Function *Main = ICF->getFunction("main");
  unordered_map<const llvm::Instruction *, BitVectorSet<const llvm::Value *>>
      Seeds;
  BitVectorSet<const llvm::Value *> Facts;
  for (unsigned Idx = 0; Idx < Main->arg_size(); ++Idx) {
    Facts.insert(getNthFunctionArgument(Main, Idx));
  }
  Seeds.insert(make_pair(&Main->front().front(), Facts));
  return Seeds;
}

void InterMonoTaintAnalysis::printNode(ostream &OS,
                                       const llvm::Instruction *N) const {
  OS << llvmIRToString(N);
}

void InterMonoTaintAnalysis::printDataFlowFact(ostream &OS,
                                               const llvm::Value *D) const {
  OS << llvmIRToString(D) << '\n';
}

void InterMonoTaintAnalysis::printFunction(ostream &OS,
                                           const llvm::Function *M) const {
  OS << M->getName().str();
}
const std::map<const llvm::Instruction *, std::set<const llvm::Value *>> &
InterMonoTaintAnalysis::getAllLeaks() const {
  return Leaks;
}

} // namespace psr
