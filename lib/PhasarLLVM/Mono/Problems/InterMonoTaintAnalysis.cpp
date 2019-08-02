/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonoTaintAnalysis.h>
#include <phasar/PhasarLLVM/Utils/TaintConfiguration.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>

using namespace std;
using namespace psr;

namespace psr {

InterMonoTaintAnalysis::InterMonoTaintAnalysis(LLVMBasedICFG &Icfg,
                                               vector<string> EntryPoints)
    : InterMonoProblem<const llvm::Instruction *, const llvm::Value *,
                       const llvm::Function *, LLVMBasedICFG &>(Icfg),
      TSF(), EntryPoints(EntryPoints) {}

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::join(const MonoSet<const llvm::Value *> &Lhs,
                             const MonoSet<const llvm::Value *> &Rhs) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "InterMonoTaintAnalysis::join()");
  // cout << "InterMonoTaintAnalysis::join()\n";
  MonoSet<const llvm::Value *> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  return Result;
}

bool InterMonoTaintAnalysis::sqSubSetEqual(
    const MonoSet<const llvm::Value *> &Lhs,
    const MonoSet<const llvm::Value *> &Rhs) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "InterMonoTaintAnalysis::sqSubSetEqual()");
  // cout << "InterMonoTaintAnalysis::sqSubSetEqual()\n";
  return includes(Rhs.begin(), Rhs.end(), Lhs.begin(), Lhs.end());
}

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::normalFlow(const llvm::Instruction *Stmt,
                                   const MonoSet<const llvm::Value *> &In) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "InterMonoTaintAnalysis::normalFlow()");
  MonoSet<const llvm::Value *> Out(In);
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

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::callFlow(const llvm::Instruction *CallSite,
                                 const llvm::Function *Callee,
                                 const MonoSet<const llvm::Value *> &In) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "InterMonoTaintAnalysis::callFlow()");
  MonoSet<const llvm::Value *> Out;
  llvm::ImmutableCallSite CS(CallSite);
  vector<const llvm::Value *> Actuals;
  vector<const llvm::Value *> Formals;
  // set up the actual parameters
  for (unsigned idx = 0; idx < CS.getNumArgOperands(); ++idx) {
    Actuals.push_back(CS.getArgOperand(idx));
  }
  // set up the formal parameters
  /* for (unsigned idx = 0; idx < Callee->arg_size(); ++idx) {
    Formals.push_back(getNthFunctionArgument(Callee, idx));
  }*/
  for (auto &arg : Callee->args()) {
    Formals.push_back(&arg);
  }
  for (unsigned idx = 0; idx < Actuals.size(); ++idx) {
    if (In.count(Actuals[idx])) {
      Out.insert(Formals[idx]);
    }
  }
  return Out;
}

MonoSet<const llvm::Value *> InterMonoTaintAnalysis::returnFlow(
    const llvm::Instruction *CallSite, const llvm::Function *Callee,
    const llvm::Instruction *ExitStmt, const llvm::Instruction *RetSite,
    const MonoSet<const llvm::Value *> &In) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "InterMonoTaintAnalysis::returnFlow()");
  MonoSet<const llvm::Value *> Out;
  if (auto Ret = llvm::dyn_cast<llvm::ReturnInst>(ExitStmt)) {
    if (In.count(Ret->getReturnValue())) {
      Out.insert(CallSite);
    }
  }
  // propagate pointer arguments to the caller, since this callee may modify
  // them
  llvm::ImmutableCallSite CS(CallSite);
  unsigned index = 0;
  for (auto &arg : Callee->args()) {
    if (arg.getType()->isPointerTy() && In.count(&arg)) {
      Out.insert(CS.getArgOperand(index));
    }
    index++;
  }
  return Out;
}

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::callToRetFlow(const llvm::Instruction *CallSite,
                                      const llvm::Instruction *RetSite,
                                      MonoSet<const llvm::Function *> Callees,
                                      const MonoSet<const llvm::Value *> &In) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "InterMonoTaintAnalysis::callToRetFlow()");
  MonoSet<const llvm::Value *> Out(In);
  llvm::ImmutableCallSite CS(CallSite);
  //-----------------------------------------------------------------------------
  // Handle virtual calls in the loop
  //-----------------------------------------------------------------------------
  for (auto Callee : Callees) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "InterMonoTaintAnalysis::callToRetFlow()::"
                  << Callee->getName().str());

    if (TSF.isSink(Callee->getName().str())) {
      for (unsigned idx = 0; idx < CS.getNumArgOperands(); ++idx) {
        if (TSF.getSink(Callee->getName().str()).isLeakedArg(idx) &&
            In.count(CS.getArgOperand(idx))) {
          cout << "FOUND LEAK AT: " << llvmIRToString(CallSite) << '\n';
          cout << "LEAKED VALUE: " << llvmIRToString(CS.getArgOperand(idx))
               << '\n'
               << endl;
          Leaks[CallSite].insert(CS.getArgOperand(idx));
        }
      }
    }
    if (TSF.isSource(Callee->getName().str())) {
      for (unsigned idx = 0; idx < CS.getNumArgOperands(); ++idx) {
        if (TSF.getSource(Callee->getName().str()).isTaintedArg(idx)) {
          Out.insert(CS.getArgOperand(idx));
        }
      }
      if (TSF.getSource(Callee->getName().str()).TaintsReturn) {
        Out.insert(CallSite);
      }
    }
  }

  // erase pointer arguments, since they are now propagated in the retFF
  for (unsigned i = 0; i < CS.getNumArgOperands(); ++i) {
    if (CS.getArgOperand(i)->getType()->isPointerTy()) {
      Out.erase(CS.getArgOperand(i));
    }
  }
  return Out;
}

MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
InterMonoTaintAnalysis::initialSeeds() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "InterMonoTaintAnalysis::initialSeeds()");
  // cout << "InterMonoTaintAnalysis::initialSeeds()\n";
  const llvm::Function *main = ICFG.getMethod("main");
  MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>> Seeds;
  MonoSet<const llvm::Value *> Facts;
  for (unsigned idx = 0; idx < main->arg_size(); ++idx) {
    Facts.insert(getNthFunctionArgument(main, idx));
  }
  Seeds.insert(make_pair(&main->front().front(), Facts));
  return Seeds;
}

void InterMonoTaintAnalysis::printNode(ostream &os,
                                       const llvm::Instruction *n) const {
  os << llvmIRToString(n);
}

void InterMonoTaintAnalysis::printDataFlowFact(ostream &os,
                                               const llvm::Value *d) const {
  os << llvmIRToString(d) << '\n';
}

void InterMonoTaintAnalysis::printMethod(ostream &os,
                                         const llvm::Function *m) const {
  os << m->getName().str();
}
const std::map<const llvm::Instruction *, std::set<const llvm::Value *>> &
InterMonoTaintAnalysis::getAllLeaks() const {
  return Leaks;
}

} // namespace psr