/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"

#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigUtilities.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include <utility>

namespace psr {

IFDSTaintAnalysis::IFDSTaintAnalysis(const LLVMProjectIRDB *IRDB,
                                     LLVMAliasInfoRef PT,
                                     const LLVMTaintConfig *Config,
                                     std::vector<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()),
      Config(Config), PT(PT) {
  assert(Config != nullptr);
  assert(PT);
}

bool IFDSTaintAnalysis::isSourceCall(const llvm::CallBase *CB,
                                     const llvm::Function *Callee) const {
  for (const auto &Arg : Callee->args()) {
    if (Config->isSource(&Arg)) {
      return true;
    }
  }
  const auto &Callback = Config->getRegisteredSourceCallBack();
  if (!Callback) {
    return false;
  }

  auto AdditionalFacts = Callback(CB);

  if (AdditionalFacts.empty()) {
    return false;
  }

  if (AdditionalFacts.count(CB)) {
    return true;
  }

  return std::any_of(CB->arg_begin(), CB->arg_end(),
                     [&AdditionalFacts](const auto &Arg) {
                       return AdditionalFacts.count(Arg.get());
                     });
}

bool IFDSTaintAnalysis::isSinkCall(const llvm::CallBase *CB,
                                   const llvm::Function *Callee) const {
  for (const auto &Arg : Callee->args()) {
    if (Config->isSink(&Arg)) {
      return true;
    }
  }
  const auto &Callback = Config->getRegisteredSinkCallBack();
  if (!Callback) {
    return false;
  }

  auto AdditionalLeaks = Callback(CB);

  if (AdditionalLeaks.empty()) {
    return false;
  }

  if (AdditionalLeaks.count(CB)) {
    return true;
  }

  return std::any_of(CB->arg_begin(), CB->arg_end(),
                     [&AdditionalLeaks](const auto &Arg) {
                       return AdditionalLeaks.count(Arg.get());
                     });
}

bool IFDSTaintAnalysis::isSanitizerCall(const llvm::CallBase * /*CB*/,
                                        const llvm::Function *Callee) const {
  return std::any_of(
      Callee->arg_begin(), Callee->arg_end(),
      [this](const auto &Arg) { return Config->isSanitizer(&Arg); });
}

void IFDSTaintAnalysis::populateWithMayAliases(std::set<d_t> &Facts) const {
  std::set<d_t> Tmp = Facts;
  for (const auto *Fact : Facts) {
    auto Aliases = PT.getAliasSet(Fact);
    Tmp.insert(Aliases->begin(), Aliases->end());
  }

  Facts = std::move(Tmp);
}

void IFDSTaintAnalysis::populateWithMustAliases(std::set<d_t> &Facts) const {
  /// TODO: Find must-aliases; Currently the AliasSet only contains
  /// may-aliases
}

IFDSTaintAnalysis::FlowFunctionPtrType IFDSTaintAnalysis::getNormalFlowFunction(
    IFDSTaintAnalysis::n_t Curr, [[maybe_unused]] IFDSTaintAnalysis::n_t Succ) {
  // If a tainted value is stored, the store location must be tainted too
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    struct TAFF : FlowFunction<IFDSTaintAnalysis::d_t> {
      const llvm::StoreInst *Store;
      TAFF(const llvm::StoreInst *S) : Store(S){};
      std::set<IFDSTaintAnalysis::d_t>
      computeTargets(IFDSTaintAnalysis::d_t Source) override {
        if (Store->getValueOperand() == Source) {
          return std::set<IFDSTaintAnalysis::d_t>{Store->getPointerOperand(),
                                                  Source};
        }
        if (Store->getValueOperand() != Source &&
            Store->getPointerOperand() == Source) {
          return {};
        }
        return {Source};
      }
    };
    return std::make_shared<TAFF>(Store);
  }
  // If a tainted value is loaded, the loaded value is of course tainted
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    return generateFlow(Load, Load->getPointerOperand());
  }
  // Check if an address is computed from a tainted base pointer of an
  // aggregated object
  if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    return generateFlow(GEP, GEP->getPointerOperand());
  }
  // Check if a tainted value is extracted and taint the targets of
  // the extract operation accordingly
  if (const auto *Extract = llvm::dyn_cast<llvm::ExtractValueInst>(Curr)) {

    return generateFlow(Extract, Extract->getAggregateOperand());
  }

  // Otherwise we do not care and leave everything as it is
  return Identity<IFDSTaintAnalysis::d_t>::getInstance();
}

IFDSTaintAnalysis::FlowFunctionPtrType
IFDSTaintAnalysis::getCallFlowFunction(IFDSTaintAnalysis::n_t CallSite,
                                       IFDSTaintAnalysis::f_t DestFun) {
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  // Check if a source or sink function is called:
  // We then can kill all data-flow facts not following the called function.
  // The respective taints or leaks are then generated in the corresponding
  // call to return flow function.
  if (isSourceCall(CS, DestFun) || isSinkCall(CS, DestFun)) {
    return killAllFlows<d_t>();
  }

  // Map the actual into the formal parameters
  return mapFactsToCallee(CS, DestFun);
}

IFDSTaintAnalysis::FlowFunctionPtrType IFDSTaintAnalysis::getRetFlowFunction(
    IFDSTaintAnalysis::n_t CallSite, IFDSTaintAnalysis::f_t /*CalleeFun*/,
    IFDSTaintAnalysis::n_t ExitStmt,
    [[maybe_unused]] IFDSTaintAnalysis::n_t RetSite) {
  // We must check if the return value and formal parameter are tainted, if so
  // we must taint all user's of the function call. We are only interested in
  // formal parameters of pointer/reference type.
  return mapFactsToCaller(
      llvm::cast<llvm::CallBase>(CallSite), ExitStmt,
      [](d_t Formal, d_t Source) {
        return Formal == Source && Formal->getType()->isPointerTy();
      },
      [](d_t RetVal, d_t Source) { return RetVal == Source; });
  // All other stuff is killed at this point
}

IFDSTaintAnalysis::FlowFunctionPtrType
IFDSTaintAnalysis::getCallToRetFlowFunction(
    IFDSTaintAnalysis::n_t CallSite,
    [[maybe_unused]] IFDSTaintAnalysis::n_t RetSite,
    llvm::ArrayRef<f_t> Callees) {
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  std::set<d_t> Gen;
  std::set<d_t> Leak;
  std::set<d_t> Kill;
  bool HasBody = false;
  // Process the effects of source or sink functions that are called
  for (const auto *Callee : Callees) {
    if (!Callee->isDeclaration()) {
      HasBody = true;
    }
    collectGeneratedFacts(Gen, *Config, CS, Callee);
    collectLeakedFacts(Leak, *Config, CS, Callee);
    collectSanitizedFacts(Kill, *Config, CS, Callee);
  }

  if (HasBody && Gen.empty() && Leak.empty() && Kill.empty()) {
    // We have a normal function-call and the ret-FF is responsible for handling
    // pointer parameters. So we need to kill them here
    for (const auto &Arg : CS->args()) {
      if (Arg->getType()->isPointerTy()) {
        Kill.insert(Arg.get());
      }
    }
  }

  populateWithMayAliases(Gen);
  populateWithMayAliases(Leak);

  Gen.insert(LLVMZeroValue::getInstance());

  populateWithMustAliases(Kill);

  if (Gen.empty() && (!Leak.empty() || !Kill.empty())) {
    return lambdaFlow<d_t>([Leak{std::move(Leak)}, Kill{std::move(Kill)}, this,
                            CallSite](d_t Source) -> std::set<d_t> {
      if (Leak.count(Source)) {
        Leaks[CallSite].insert(Source);
      }

      if (Kill.count(Source)) {
        return {};
      }

      return {Source};
    });
  }
  if (Kill.empty()) {
    return lambdaFlow<d_t>([Gen{std::move(Gen)}, Leak{std::move(Leak)}, this,
                            CallSite](d_t Source) -> std::set<d_t> {
      if (LLVMZeroValue::isLLVMZeroValue(Source)) {
        return Gen;
      }

      if (Leak.count(Source)) {
        Leaks[CallSite].insert(Source);
      }

      return {Source};
    });
  }
  return lambdaFlow<d_t>([Gen{std::move(Gen)}, Leak{std::move(Leak)},
                          Kill{std::move(Kill)}, this,
                          CallSite](d_t Source) -> std::set<d_t> {
    if (LLVMZeroValue::isLLVMZeroValue(Source)) {
      return Gen;
    }

    if (Leak.count(Source)) {
      Leaks[CallSite].insert(Source);
    }

    if (Kill.count(Source)) {
      return {};
    }

    return {Source};
  });

  // Otherwise pass everything as it is
  return Identity<IFDSTaintAnalysis::d_t>::getInstance();
}

IFDSTaintAnalysis::FlowFunctionPtrType
IFDSTaintAnalysis::getSummaryFlowFunction(
    [[maybe_unused]] IFDSTaintAnalysis::n_t CallSite,
    [[maybe_unused]] IFDSTaintAnalysis::f_t DestFun) {
  // $sSS1poiyS2S_SStFZ is Swift's String append method
  // if concat a tainted string with something else the
  // result should be tainted
  if (DestFun->getName().equals("$sSS1poiyS2S_SStFZ")) {
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);

    return generateFlowIf<d_t>(CallSite, [CS](d_t Source) {
      return ((Source == CS->getArgOperand(1)) ||
              (Source == CS->getArgOperand(3)));
    });
  }
  return nullptr;
}

InitialSeeds<IFDSTaintAnalysis::n_t, IFDSTaintAnalysis::d_t,
             IFDSTaintAnalysis::l_t>
IFDSTaintAnalysis::initialSeeds() {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSTaintAnalysis::initialSeeds()");
  // If main function is the entry point, commandline arguments have to be
  // tainted. Otherwise we just use the zero value to initialize the analysis.
  InitialSeeds<n_t, d_t, l_t> Seeds;

  LLVMBasedCFG C;
  forallStartingPoints(EntryPoints, IRDB, C, [this, &Seeds](n_t SP) {
    Seeds.addSeed(SP, getZeroValue());
    if (SP->getFunction()->getName() == "main") {
      std::set<IFDSTaintAnalysis::d_t> CmdArgs;
      for (const auto &Arg : SP->getFunction()->args()) {
        Seeds.addSeed(SP, &Arg);
      }
    }
  });

  return Seeds;
}

IFDSTaintAnalysis::d_t IFDSTaintAnalysis::createZeroValue() const {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSTaintAnalysis::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSTaintAnalysis::isZeroValue(IFDSTaintAnalysis::d_t FlowFact) const {
  return LLVMZeroValue::isLLVMZeroValue(FlowFact);
}

void IFDSTaintAnalysis::printNode(llvm::raw_ostream &Os,
                                  IFDSTaintAnalysis::n_t Inst) const {
  Os << llvmIRToString(Inst);
}

void IFDSTaintAnalysis::printDataFlowFact(
    llvm::raw_ostream &Os, IFDSTaintAnalysis::d_t FlowFact) const {
  Os << llvmIRToString(FlowFact);
}

void IFDSTaintAnalysis::printFunction(llvm::raw_ostream &Os,
                                      IFDSTaintAnalysis::f_t Fun) const {
  Os << Fun->getName();
}

void IFDSTaintAnalysis::emitTextReport(
    const SolverResults<n_t, d_t, BinaryDomain> & /*SR*/,
    llvm::raw_ostream &OS) {
  OS << "\n----- Found the following leaks -----\n";
  if (Leaks.empty()) {
    OS << "No leaks found!\n";
  } else {
    for (const auto &Leak : Leaks) {
      OS << "At instruction\nIR  : " << llvmIRToString(Leak.first) << '\n';
      OS << "\n\nLeak(s):\n";
      for (const auto *LeakedValue : Leak.second) {
        OS << "IR  : ";
        // Get the actual leaked alloca instruction if possible
        if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(LeakedValue)) {
          OS << llvmIRToString(Load->getPointerOperand()) << '\n';
        } else {
          OS << llvmIRToString(LeakedValue) << '\n';
        }
      }
      OS << "-------------------\n";
    }
  }
}

} // namespace psr
