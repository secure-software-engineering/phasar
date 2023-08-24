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
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
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

void IFDSTaintAnalysis::populateWithMayAliases(
    container_type &Facts, const llvm::Instruction *Context) const {
  container_type Tmp = Facts;
  for (const auto *Fact : Facts) {
    auto Aliases = PT.getAliasSet(Fact);
    for (const auto *Alias : *Aliases) {
      if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(Alias)) {
        /// Mapping instructions between functions is done via the call-FF and
        /// ret-FF
        if (Inst->getFunction() != Context->getFunction()) {
          continue;
        }
        if (Inst->getParent() == Context->getParent() &&
            Context->comesBefore(Inst)) {
          // We will see that inst later
          continue;
        }
      } else if (const auto *Glob =
                     llvm::dyn_cast<llvm::GlobalVariable>(Alias)) {
        if (Glob != Fact && Glob->isConstant()) {
          // Data cannot flow into the readonly-data section
          continue;
        }
      }

      Tmp.insert(Alias);
    }
  }

  Facts = std::move(Tmp);
}

void IFDSTaintAnalysis::populateWithMustAliases(
    container_type &Facts, const llvm::Instruction *Context) const {
  /// TODO: Find must-aliases; Currently the AliasSet only contains
  /// may-aliases
}

auto IFDSTaintAnalysis::getNormalFlowFunction(n_t Curr,
                                              [[maybe_unused]] n_t Succ)
    -> FlowFunctionPtrType {
  // If a tainted value is stored, the store location must be tainted too
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    container_type Gen;
    Gen.insert(Store->getPointerOperand());
    populateWithMayAliases(Gen, Store);
    Gen.insert(Store->getValueOperand());

    return lambdaFlow(
        [Store, Gen{std::move(Gen)}](d_t Source) -> container_type {
          if (Store->getValueOperand() == Source) {
            return Gen;
          }
          if (Store->getPointerOperand() == Source) {
            return {};
          }
          return {Source};
        });
  }
  // If a tainted value is loaded, the loaded value is of course tainted
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    return transferFlow(Load, Load->getPointerOperand());
  }
  // Check if an address is computed from a tainted base pointer of an
  // aggregated object
  if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    return transferFlow(GEP, GEP->getPointerOperand());
  }
  // Check if a tainted value is extracted and taint the targets of
  // the extract operation accordingly
  if (const auto *Extract = llvm::dyn_cast<llvm::ExtractValueInst>(Curr)) {
    return transferFlow(Extract, Extract->getAggregateOperand());
  }

  if (const auto *Insert = llvm::dyn_cast<llvm::InsertValueInst>(Curr)) {
    return lambdaFlow([Insert](d_t Source) -> container_type {
      if (Source == Insert->getAggregateOperand() ||
          Source == Insert->getInsertedValueOperand()) {
        return {Source, Insert};
      }

      if (Source == Insert) {
        return {};
      }

      return {Source};
    });
  }

  if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(Curr)) {
    return transferFlow(Cast, Cast->getOperand(0));
  }

  // Otherwise we do not care and leave everything as it is
  return identityFlow();
}

auto IFDSTaintAnalysis::getCallFlowFunction(n_t CallSite, f_t DestFun)
    -> FlowFunctionPtrType {
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  // Check if a source or sink function is called:
  // We then can kill all data-flow facts not following the called function.
  // The respective taints or leaks are then generated in the corresponding
  // call to return flow function.
  if (isSourceCall(CS, DestFun) || isSinkCall(CS, DestFun)) {
    return killAllFlows();
  }

  // Map the actual into the formal parameters
  return mapFactsToCallee(CS, DestFun);
}

auto IFDSTaintAnalysis::getRetFlowFunction(n_t CallSite, f_t /*CalleeFun*/,
                                           n_t ExitStmt,
                                           [[maybe_unused]] n_t RetSite)
    -> FlowFunctionPtrType {
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

auto IFDSTaintAnalysis::getCallToRetFlowFunction(n_t CallSite,
                                                 [[maybe_unused]] n_t RetSite,
                                                 llvm::ArrayRef<f_t> Callees)
    -> FlowFunctionPtrType {

  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);

  bool HasDeclOnly = llvm::any_of(
      Callees, [](const auto *DestFun) { return DestFun->isDeclaration(); });

  return mapFactsAlongsideCallSite(CS, [HasDeclOnly](d_t Arg) {
    return HasDeclOnly || !Arg->getType()->isPointerTy();
  });
}

auto IFDSTaintAnalysis::getSummaryFlowFunction([[maybe_unused]] n_t CallSite,
                                               [[maybe_unused]] f_t DestFun)
    -> FlowFunctionPtrType {
  // $sSS1poiyS2S_SStFZ is Swift's String append method
  // if concat a tainted string with something else the
  // result should be tainted
  if (DestFun->getName().equals("$sSS1poiyS2S_SStFZ")) {
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);

    return generateFlowIf(CallSite, [CS](d_t Source) {
      return ((Source == CS->getArgOperand(1)) ||
              (Source == CS->getArgOperand(3)));
    });
  }

  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  container_type Gen;
  container_type Leak;
  container_type Kill;

  // Process the effects of source or sink functions that are called
  collectGeneratedFacts(Gen, *Config, CS, DestFun);
  collectLeakedFacts(Leak, *Config, CS, DestFun);
  collectSanitizedFacts(Kill, *Config, CS, DestFun);

  populateWithMayAliases(Gen, CallSite);
  /// We now generate all aliases within the flow functions as facts, so we can
  /// safely just check for the sink values here
  // populateWithMayAliases(Leak, CallSite);
  populateWithMustAliases(Kill, CallSite);

  if (CS->hasStructRetAttr()) {
    const auto *SRet = CS->getArgOperand(0);
    if (!Gen.count(SRet)) {
      // SRet is guaranteed to be written to by the call. If it does not
      // generate it, we can freely kill it
      Kill.insert(SRet);
    }
  }

  if (Gen.empty()) {
    if (!Leak.empty() || !Kill.empty()) {
      return lambdaFlow([Leak{std::move(Leak)}, Kill{std::move(Kill)}, this,
                         CallSite](d_t Source) -> container_type {
        if (Leak.count(Source)) {
          Leaks[CallSite].insert(Source);
        }

        if (Kill.count(Source)) {
          return {};
        }

        return {Source};
      });
    }

    // all empty
    return nullptr;
  }

  // Gen nonempty

  Gen.insert(LLVMZeroValue::getInstance());
  return lambdaFlow([Gen{std::move(Gen)}, Leak{std::move(Leak)}, this,
                     CallSite](d_t Source) -> container_type {
    if (LLVMZeroValue::isLLVMZeroValue(Source)) {
      return Gen;
    }

    if (Leak.count(Source)) {
      Leaks[CallSite].insert(Source);
    }

    return {Source};
  });
}

auto IFDSTaintAnalysis::initialSeeds() -> InitialSeeds<n_t, d_t, l_t> {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSTaintAnalysis::initialSeeds()");
  // If main function is the entry point, commandline arguments have to be
  // tainted. Otherwise we just use the zero value to initialize the analysis.
  InitialSeeds<n_t, d_t, l_t> Seeds;

  LLVMBasedCFG C;
  forallStartingPoints(EntryPoints, IRDB, C, [this, &Seeds](n_t SP) {
    Seeds.addSeed(SP, getZeroValue());
    if (SP->getFunction()->getName() == "main") {
      for (const auto &Arg : SP->getFunction()->args()) {
        Seeds.addSeed(SP, &Arg);
      }
    }
  });

  return Seeds;
}

auto IFDSTaintAnalysis::createZeroValue() const -> d_t {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSTaintAnalysis::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSTaintAnalysis::isZeroValue(d_t FlowFact) const noexcept {
  return LLVMZeroValue::isLLVMZeroValue(FlowFact);
}

void IFDSTaintAnalysis::emitTextReport(
    const SolverResults<n_t, d_t, BinaryDomain> & /*SR*/,
    llvm::raw_ostream &OS) {
  OS << "\n----- Found the following leaks -----\n";
  if (Leaks.empty()) {
    OS << "No leaks found!\n";
    return;
  }

  for (const auto &Leak : Leaks) {
    OS << "At instruction\nIR  : " << llvmIRToString(Leak.first) << '\n';
    OS << "\nLeak(s):\n";
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

} // namespace psr
