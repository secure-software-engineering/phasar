/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"

#include "phasar/DataFlow/IfdsIde/EntryPointUtils.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LibCSummary.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigUtilities.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
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
using d_t = IFDSTaintAnalysis::d_t;
using container_type = IFDSTaintAnalysis::container_type;

IFDSTaintAnalysis::IFDSTaintAnalysis(const LLVMProjectIRDB *IRDB,
                                     LLVMAliasInfoRef PT,
                                     const LLVMTaintConfig *Config,
                                     std::vector<std::string> EntryPoints,
                                     bool TaintMainArgs)
    : IFDSTabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()),
      Config(Config), PT(PT), TaintMainArgs(TaintMainArgs),
      Llvmfdff(library_summary::readFromFDFF(getLibCSummary(), *IRDB)) {
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

static bool canSkipAtContext(const llvm::Value *Val,
                             const llvm::Instruction *Context) noexcept {
  if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(Val)) {
    /// Mapping instructions between functions is done via the call-FF and
    /// ret-FF
    if (Inst->getFunction() != Context->getFunction()) {
      return true;
    }
    if (Inst->getParent() == Context->getParent() &&
        Context->comesBefore(Inst)) {
      // We will see that inst later
      return true;
    }
    return false;
  }

  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(Val)) {
    // An argument is only valid in the function it belongs to
    if (Arg->getParent() != Context->getFunction()) {
      return true;
    }
  }
  return false;
}

static bool isCompiletimeConstantData(const llvm::Value *Val) noexcept {
  if (const auto *Glob = llvm::dyn_cast<llvm::GlobalVariable>(Val)) {
    // Data cannot flow into the readonly-data section
    return Glob->isConstant();
  }

  return llvm::isa<llvm::Function>(Val) || llvm::isa<llvm::ConstantData>(Val);
}

void IFDSTaintAnalysis::populateWithMayAliases(
    container_type &Facts, const llvm::Instruction *Context) const {
  container_type Tmp = Facts;
  for (const auto *Fact : Facts) {
    auto Aliases = PT.getAliasSet(Fact);
    for (const auto *Alias : *Aliases) {
      if (canSkipAtContext(Alias, Context)) {
        continue;
      }

      if (isCompiletimeConstantData(Alias)) {
        continue;
      }

      if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Alias)) {
        // Handle at least one level of indirection...
        const auto *PointerOp = Load->getPointerOperand()->stripPointerCasts();
        Tmp.insert(PointerOp);
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

static IFDSTaintAnalysis::FlowFunctionPtrType transferAndKillFlow(d_t To,
                                                                  d_t From) {
  if (From->hasNUsesOrMore(2)) {
    return FlowFunctionTemplates<d_t, container_type>::transferFlow(To, From);
  }
  return FlowFunctionTemplates<d_t, container_type>::lambdaFlow(
      [To, From](d_t Source) -> container_type {
        if (Source == From) {
          return {To};
        }
        if (Source == To) {
          return {};
        }
        return {Source};
      });
}

static IFDSTaintAnalysis::FlowFunctionPtrType
transferAndKillTwoFlows(d_t To, d_t From1, d_t From2) {
  bool KillFrom1 = !From1->hasNUsesOrMore(2);
  bool KillFrom2 = !From2->hasNUsesOrMore(2);

  if (KillFrom1) {
    if (KillFrom2) {
      return FlowFunctionTemplates<d_t, container_type>::lambdaFlow(
          [To, From1, From2](d_t Source) -> container_type {
            if (Source == From1 || Source == From2) {
              return {To};
            }
            if (Source == To) {
              return {};
            }
            return {Source};
          });
    }

    return FlowFunctionTemplates<d_t, container_type>::lambdaFlow(
        [To, From1, From2](d_t Source) -> container_type {
          if (Source == From1) {
            return {To};
          }
          if (Source == From2) {
            return {Source, To};
          }
          if (Source == To) {
            return {};
          }
          return {Source};
        });
  }

  if (KillFrom2) {
    return FlowFunctionTemplates<d_t, container_type>::lambdaFlow(
        [To, From1, From2](d_t Source) -> container_type {
          if (Source == From1) {
            return {Source, To};
          }
          if (Source == From2) {
            return {To};
          }
          if (Source == To) {
            return {};
          }
          return {Source};
        });
  }

  return FlowFunctionTemplates<d_t, container_type>::lambdaFlow(
      [To, From1, From2](d_t Source) -> container_type {
        if (Source == From1 || Source == From2) {
          return {Source, To};
        }
        if (Source == To) {
          return {};
        }
        return {Source};
      });
}

auto IFDSTaintAnalysis::getNormalFlowFunction(
    n_t Curr, [[maybe_unused]] n_t Succ) -> FlowFunctionPtrType {
  // If a tainted value is stored, the store location must be tainted too
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    container_type Gen;
    Gen.insert(Store->getPointerOperand());
    populateWithMayAliases(Gen, Store);
    if (Store->getValueOperand()->hasNUsesOrMore(2)) {
      Gen.insert(Store->getValueOperand());
    }

    return lambdaFlow(
        [Store, Gen{std::move(Gen)}](d_t Source) -> container_type {
          if (Store->getPointerOperand() == Source) {
            return {};
          }
          if (Store->getValueOperand() == Source) {
            return Gen;
          }

          return {Source};
        });
  }
  // If a tainted value is loaded, the loaded value is of course tainted
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    return transferAndKillFlow(Load, Load->getPointerOperand());
  }
  // Check if an address is computed from a tainted base pointer of an
  // aggregated object
  if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    return transferAndKillFlow(GEP, GEP->getPointerOperand());
  }
  // Check if a tainted value is extracted and taint the targets of
  // the extract operation accordingly
  if (const auto *Extract = llvm::dyn_cast<llvm::ExtractValueInst>(Curr)) {
    return transferAndKillFlow(Extract, Extract->getAggregateOperand());
  }

  if (const auto *Insert = llvm::dyn_cast<llvm::InsertValueInst>(Curr)) {
    return transferAndKillTwoFlows(Insert, Insert->getAggregateOperand(),
                                   Insert->getInsertedValueOperand());
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

auto IFDSTaintAnalysis::getRetFlowFunction(
    n_t CallSite, f_t /*CalleeFun*/, n_t ExitStmt,
    [[maybe_unused]] n_t RetSite) -> FlowFunctionPtrType {
  // We must check if the return value and formal parameter are tainted, if so
  // we must taint all user's of the function call. We are only interested in
  // formal parameters of pointer/reference type.
  return mapFactsToCaller(
      llvm::cast<llvm::CallBase>(CallSite), ExitStmt,
      [](d_t Formal, d_t Source) {
        return Formal == Source && Formal->getType()->isPointerTy();
      },
      [](d_t RetVal, d_t Source) { return RetVal == Source; }, {}, true, true,
      [this, CallSite](container_type &Res) {
        // Correctly handling return-POIs
        populateWithMayAliases(Res, CallSite);
      });
  // All other stuff is killed at this point
}

auto IFDSTaintAnalysis::getCallToRetFlowFunction(
    n_t CallSite, [[maybe_unused]] n_t RetSite,
    llvm::ArrayRef<f_t> Callees) -> FlowFunctionPtrType {

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
  if (Gen.empty() && Leak.empty() && Kill.empty()) {
    if (Llvmfdff.contains(DestFun)) {
      // Note: The LLVMfdff is constant during the lifetime of the analysis, so
      // it is fine to capture a reference here:
      const auto &DestFunFacts = Llvmfdff.getFactsForFunction(DestFun);
      return lambdaFlow([CallSite, DestFun,
                         &DestFunFacts](d_t Source) -> container_type {
        std::set<d_t> Facts;
        const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
        for (const auto &[Arg, DestParam] :
             llvm::zip(CS->args(), DestFun->args())) {
          if (Source == Arg.get()) {
            auto VecFacts = DestFunFacts.find(DestParam.getArgNo());
            if (VecFacts != DestFunFacts.end()) {
              for (const auto &VecFact : VecFacts->second) {
                if (const auto *Param = std::get_if<library_summary::Parameter>(
                        &VecFact.Fact)) {
                  Facts.insert(CS->getArgOperand(Param->Index));
                } else {
                  Facts.insert(CallSite);
                }
              }
            }
          }
        }
        Facts.insert(Source);
        return Facts;
      });
    }

    // not found
    return nullptr;
  }
  if (Gen.empty()) {
    if (!Leak.empty() || !Kill.empty()) {
      return lambdaFlow([Leak{std::move(Leak)}, Kill{std::move(Kill)}, this,
                         CallSite](d_t Source) -> container_type {
        if (Leak.count(Source)) {
          if (Leaks[CallSite].insert(Source).second) {
            Printer->onResult(CallSite, Source,
                              DataFlowAnalysisType::IFDSTaintAnalysis);
          }
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
      if (Leaks[CallSite].insert(Source).second) {
        Printer->onResult(CallSite, Source,
                          DataFlowAnalysisType::IFDSTaintAnalysis);
      }
    }

    return {Source};
  });
}

auto IFDSTaintAnalysis::initialSeeds() -> InitialSeeds<n_t, d_t, l_t> {
  PHASAR_LOG_LEVEL(DEBUG, "IFDSTaintAnalysis::initialSeeds()");

  InitialSeeds<n_t, d_t, l_t> Seeds;

  LLVMBasedCFG C;
  addSeedsForStartingPoints(EntryPoints, IRDB, C, Seeds, getZeroValue(),
                            psr::BinaryDomain::BOTTOM);

  if (TaintMainArgs && llvm::is_contained(EntryPoints, "main")) {
    // If main function is the entry point, commandline arguments have to be
    // tainted. Otherwise we just use the zero value to initialize the analysis.

    const auto *MainF = IRDB->getFunction("main");
    for (const auto *SP : C.getStartPointsOf(MainF)) {
      for (const auto &Arg : SP->getFunction()->args()) {
        Seeds.addSeed(SP, &Arg);
      }
    }
  }

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
  Printer->onFinalize();
}

} // namespace psr
