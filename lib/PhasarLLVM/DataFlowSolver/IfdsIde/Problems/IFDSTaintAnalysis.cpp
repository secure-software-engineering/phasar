/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <utility>

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/SpecialSummaries.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigUtilities.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace std;
using namespace psr;

namespace psr {

IFDSTaintAnalysis::IFDSTaintAnalysis(const ProjectIRDB *IRDB,
                                     const LLVMTypeHierarchy *TH,
                                     const LLVMBasedICFG *ICF,
                                     LLVMPointsToInfo *PT,
                                     const TaintConfig &Config,
                                     std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)),
      Config(Config) {
  IFDSTaintAnalysis::ZeroValue = IFDSTaintAnalysis::createZeroValue();
}

bool IFDSTaintAnalysis::isSourceCall(const llvm::CallBase *CB,
                                     const llvm::Function *Callee) const {
  for (const auto &Arg : Callee->args()) {
    if (Config.isSource(&Arg)) {
      return true;
    }
  }
  auto Callback = Config.getRegisteredSourceCallBack();
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
    if (Config.isSink(&Arg)) {
      return true;
    }
  }
  auto Callback = Config.getRegisteredSinkCallBack();
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
      [this](const auto &Arg) { return Config.isSanitizer(&Arg); });
}

void IFDSTaintAnalysis::populateWithMayAliases(std::set<d_t> &Facts) const {
  std::set<d_t> Tmp = Facts;
  for (const auto *Fact : Facts) {
    auto Aliases = PT->getPointsToSet(Fact);
    Tmp.insert(Aliases->begin(), Aliases->end());
  }

  Facts = std::move(Tmp);
}

void IFDSTaintAnalysis::populateWithMustAliases(std::set<d_t> &Facts) const {
  /// TODO: Find must-aliases; Currently the PointsToSet only contains
  /// may-aliases
}

IFDSTaintAnalysis::FlowFunctionPtrType IFDSTaintAnalysis::getNormalFlowFunction(
    IFDSTaintAnalysis::n_t Curr, [[maybe_unused]] IFDSTaintAnalysis::n_t Succ) {
  // If a tainted value is stored, the store location must be tainted too
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    struct TAFF : FlowFunction<IFDSTaintAnalysis::d_t> {
      const llvm::StoreInst *Store;
      TAFF(const llvm::StoreInst *S) : Store(S){};
      set<IFDSTaintAnalysis::d_t>
      computeTargets(IFDSTaintAnalysis::d_t Source) override {
        if (Store->getValueOperand() == Source) {
          return set<IFDSTaintAnalysis::d_t>{Store->getPointerOperand(),
                                             Source};
        }
        if (Store->getValueOperand() != Source &&
            Store->getPointerOperand() == Source) {
          return {};
        }
        return {Source};
      }
    };
    return make_shared<TAFF>(Store);
  }
  // If a tainted value is loaded, the loaded value is of course tainted
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    return make_shared<GenIf<IFDSTaintAnalysis::d_t>>(
        Load, [Load](IFDSTaintAnalysis::d_t Source) {
          return Source == Load->getPointerOperand();
        });
  }
  // Check if an address is computed from a tainted base pointer of an
  // aggregated object
  if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    return make_shared<GenIf<IFDSTaintAnalysis::d_t>>(
        GEP, [GEP](IFDSTaintAnalysis::d_t Source) {
          return Source == GEP->getPointerOperand();
        });
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
    return KillAll<IFDSTaintAnalysis::d_t>::getInstance();
  }
  // Map the actual into the formal parameters
  return make_shared<MapFactsToCallee<>>(CS, DestFun);
}

IFDSTaintAnalysis::FlowFunctionPtrType IFDSTaintAnalysis::getRetFlowFunction(
    IFDSTaintAnalysis::n_t CallSite, IFDSTaintAnalysis::f_t CalleeFun,
    IFDSTaintAnalysis::n_t ExitStmt,
    [[maybe_unused]] IFDSTaintAnalysis::n_t RetSite) {
  // We must check if the return value and formal parameter are tainted, if so
  // we must taint all user's of the function call. We are only interested in
  // formal parameters of pointer/reference type.
  return make_shared<MapFactsToCaller<>>(
      llvm::cast<llvm::CallBase>(CallSite), CalleeFun, ExitStmt, true,
      [](IFDSTaintAnalysis::d_t Formal) {
        return Formal->getType()->isPointerTy();
      });
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
    collectGeneratedFacts(Gen, Config, CS, Callee);
    collectLeakedFacts(Leak, Config, CS, Callee);
    collectSanitizedFacts(Kill, Config, CS, Callee);
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
    return makeLambdaFlow<d_t>([Leak{std::move(Leak)}, Kill{std::move(Kill)},
                                this, CallSite](d_t Source) -> std::set<d_t> {
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
    return makeLambdaFlow<d_t>([Gen{std::move(Gen)}, Leak{std::move(Leak)},
                                this, CallSite](d_t Source) -> std::set<d_t> {
      if (LLVMZeroValue::isLLVMZeroValue(Source)) {
        return Gen;
      }

      if (Leak.count(Source)) {
        Leaks[CallSite].insert(Source);
      }

      return {Source};
    });
  }
  return makeLambdaFlow<d_t>([Gen{std::move(Gen)}, Leak{std::move(Leak)},
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
  // Don't use a special summary
  return nullptr;
}

InitialSeeds<IFDSTaintAnalysis::n_t, IFDSTaintAnalysis::d_t,
             IFDSTaintAnalysis::l_t>
IFDSTaintAnalysis::initialSeeds() {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSTaintAnalysis::initialSeeds()");
  // If main function is the entry point, commandline arguments have to be
  // tainted. Otherwise we just use the zero value to initialize the analysis.
  InitialSeeds<IFDSTaintAnalysis::n_t, IFDSTaintAnalysis::d_t,
               IFDSTaintAnalysis::l_t>
      Seeds;
  for (const auto &EntryPoint : EntryPoints) {
    Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(),
                  getZeroValue());
    if (EntryPoint == "main") {
      set<IFDSTaintAnalysis::d_t> CmdArgs;
      for (const auto &Arg : ICF->getFunction(EntryPoint)->args()) {
        Seeds.addSeed(&ICF->getFunction(EntryPoint)->front().front(), &Arg);
      }
    }
  }
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
