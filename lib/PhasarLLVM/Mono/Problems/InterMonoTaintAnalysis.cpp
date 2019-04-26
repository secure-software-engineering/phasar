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
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h>
#include <phasar/PhasarLLVM/Mono/Problems/InterMonoTaintAnalysis.h>
#include <phasar/PhasarLLVM/Utils/TaintSensitiveFunctions.h>
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
      EntryPoints(EntryPoints) {}

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::join(const MonoSet<const llvm::Value *> &Lhs,
                             const MonoSet<const llvm::Value *> &Rhs) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "InterMonoTaintAnalysis::join()");
  // cout << "InterMonoTaintAnalysis::join()\n";
  MonoSet<const llvm::Value *> Result;
  set_union(Lhs.begin(), Lhs.end(), Rhs.begin(), Rhs.end(),
            inserter(Result, Result.begin()));
  cout << "Result size from join: " << Result.size() << endl;
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
  // cout << "InterMonoTaintAnalysis::normalFlow()\n";
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Store = llvm::dyn_cast<llvm::StoreInst>(Stmt)) {
    for (auto *source : In) {
      if (Store->getValueOperand() == source) {
        Result.insert(Store);
        return Result;
      }
    }
  }
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(Stmt)) {
    for (auto *source : In) {
      if (Load->getPointerOperand() == source) {
        Result.insert(Load);
        return Result;
      }
    }
  }
  if (auto GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Stmt)) {
    for (auto *source : In) {
      if (GEP->getPointerOperand() == source) {
        Result.insert(GEP);
        return Result;
      }
    }
  }
  cout << "Result size from NormalFlow: " << Result.size() << endl;
  return Result;
}

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::callFlow(const llvm::Instruction *CallSite,
                                 const llvm::Function *Callee,
                                 const MonoSet<const llvm::Value *> &In) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "InterMonoTaintAnalysis::callFlow()");
  // cout << "InterMonoTaintAnalysis::callFlow()\n";
  TaintSensitiveFunctions TSF;
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  cout << "CallSite inst is from callflow: " << endl;
  cout << CallSite << "\n";
  if (const auto Call = llvm::dyn_cast<llvm::CallInst>(CallSite)) {
    if (TSF.isSource(Callee->getName()) || (TSF.isSink(Callee->getName()))) {
      cout << "it should be reported as tainted: " << Result.size() << endl;
      Result.insert(CallSite);
    }
    llvm::Function *F =
        llvm::dyn_cast<llvm::CallInst>(CallSite)->getCalledFunction();
    cout << "FunctionName call inst: " << F->getName().str() << endl;
    string FunctionName = Callee->getName().str();
    cout << "FunctionName: " << FunctionName << endl;
    cout << "Result size from callFlow: " << Result.size() << endl;
    if (llvm::isa<llvm::CallInst>(CallSite) ||
        llvm::isa<llvm::InvokeInst>(CallSite)) {
      make_shared<MapFactsToCallee>(llvm::ImmutableCallSite(CallSite), Callee);
    }
    return Result;
  }
}

MonoSet<const llvm::Value *> InterMonoTaintAnalysis::returnFlow(
    const llvm::Instruction *CallSite, const llvm::Function *Callee,
    const llvm::Instruction *RetSite, const MonoSet<const llvm::Value *> &In) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "InterMonoTaintAnalysis::returnFlow()");

  TaintSensitiveFunctions TSF;
  MonoSet<const llvm::Value *> Result;
  Result.insert(In.begin(), In.end());
  if (const auto Return = llvm::dyn_cast<llvm::ReturnInst>(RetSite)) {
    if (TSF.isSource(Return->getName()) || (TSF.isSink(Return->getName()))) {
      Result.insert(Return->getReturnValue());
    }
  }
  make_shared<MapFactsToCaller>(llvm::ImmutableCallSite(CallSite), Callee,
                                RetSite, [](const llvm::Value *formal) {
                                  return formal->getType()->isPointerTy();
                                });
  cout << "Result size from returnFlow: " << Result.size() << endl;
  return Result;
}

MonoSet<const llvm::Value *>
InterMonoTaintAnalysis::callToRetFlow(const llvm::Instruction *CallSite,
                                      const llvm::Instruction *RetSite,
                                      const MonoSet<const llvm::Value *> &In) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "InterMonoTaintAnalysis::callToRetFlow()");
  // cout << "InterMonoTaintAnalysis::callToRetFlow()\n";
  TaintSensitiveFunctions TSF;
  MonoSet<const llvm::Value *> Result;

  for (auto *Callee : ICFG.getCalleesOfCallAt(CallSite)) {
    string FunctionName = cxx_demangle(Callee->getName().str());
    if (TSF.isSource(FunctionName) || TSF.isSink(FunctionName)) {
      auto Source = TSF.getSource(FunctionName);
      llvm::ImmutableCallSite callSite(CallSite);
      for (auto FormalIndex : Source.TaintedArgs) {
        const llvm::Value *val = callSite.getArgOperand(FormalIndex);
        Result.insert(val);
      }
      if (Source.TaintsReturn) {
        Result.insert(CallSite);
      }
      return Result;
    }
  }
  return In;
}

MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
InterMonoTaintAnalysis::initialSeeds() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "InterMonoTaintAnalysis::initialSeeds()");
  // cout << "InterMonoTaintAnalysis::initialSeeds()\n";
  const llvm::Function *main = ICFG.getMethod("main");
  MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>> Seeds;
  Seeds.insert(
      make_pair(&main->front().front(), MonoSet<const llvm::Value *>()));
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

} // namespace psr