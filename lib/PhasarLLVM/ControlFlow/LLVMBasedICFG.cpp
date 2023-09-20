/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"

#include "phasar/Config/Configuration.h"
#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMBasedContainerConfig.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Soundness.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/ErrorHandling.h"

#include <cstdint>
#include <utility>

namespace psr {
struct LLVMBasedICFG::Builder {
  LLVMProjectIRDB *IRDB = nullptr;
  LLVMAliasInfoRef PT{};
  LLVMTypeHierarchy *TH{};
  CallGraphBuilder<const llvm::Instruction *, const llvm::Function *>
      CGBuilder{};
  std::unique_ptr<Resolver> Res = nullptr;
  llvm::DenseSet<const llvm::Function *> VisitedFunctions{};
  llvm::SmallVector<llvm::Function *, 1> UserEntryPoints{};

  llvm::Function *GlobalCleanupFn = nullptr;

  llvm::SmallDenseMap<const llvm::Module *, llvm::Function *>
      GlobalRegisteredDtorsCaller{};

  // The worklist for direct callee resolution.
  llvm::SmallVector<const llvm::Function *, 0> FunctionWL{};

  // Map indirect calls to the number of possible targets found for it. Fixpoint
  // is not reached when more targets are found.
  llvm::DenseMap<const llvm::Instruction *, unsigned> IndirectCalls{};

  void initEntryPoints(llvm::ArrayRef<std::string> EntryPoints);
  void initGlobalsAndWorkList(LLVMBasedICFG *ICFG, bool IncludeGlobals);
  [[nodiscard]] CallGraph<const llvm::Instruction *, const llvm::Function *>
  buildCallGraph(Soundness S);

  /// \returns FixPointReached
  bool processFunction(/*bidigraph_t &Callgraph,*/ const llvm::Function *F);
  /// \returns FoundNewTargets
  bool constructDynamicCall(const llvm::Instruction *CS);
};

void LLVMBasedICFG::Builder::initEntryPoints(
    llvm::ArrayRef<std::string> EntryPoints) {
  if (EntryPoints.size() == 1 && EntryPoints.front() == "__ALL__") {
    UserEntryPoints.reserve(IRDB->getNumFunctions());
    // Handle the special case in which a user wishes to treat all functions as
    // entry points.
    for (const auto *Fun : IRDB->getAllFunctions()) {
      // Only functions with external linkage (or 'main') can be called from the
      // outside!
      if (!Fun->isDeclaration() && Fun->hasName() &&
          (Fun->hasExternalLinkage() || Fun->getName() == "main")) {
        UserEntryPoints.push_back(IRDB->getFunction(Fun->getName()));
      }
    }
  } else {
    UserEntryPoints.reserve(EntryPoints.size());
    for (const auto &EntryPoint : EntryPoints) {
      auto *F = IRDB->getFunctionDefinition(EntryPoint);
      if (F == nullptr) {
        PHASAR_LOG_LEVEL(WARNING,
                         "Could not retrieve function for entry point '"
                             << EntryPoint << "'");
        continue;
      }
      UserEntryPoints.push_back(F);
    }
  }
}

void LLVMBasedICFG::Builder::initGlobalsAndWorkList(LLVMBasedICFG *ICFG,
                                                    bool IncludeGlobals) {
  FunctionWL.reserve(IRDB->getNumFunctions());
  if (IncludeGlobals) {
    const auto *GlobCtor = ICFG->buildCRuntimeGlobalCtorsDtorsModel(
        *IRDB->getModule(), UserEntryPoints);
    FunctionWL.push_back(GlobCtor);
  } else {
    FunctionWL.insert(FunctionWL.end(), UserEntryPoints.begin(),
                      UserEntryPoints.end());
  }
  // Note: Pre-allocate the call-graph builder *after* adding the
  // CRuntimeGlobalCtorsDtorsModel
  CGBuilder.reserve(IRDB->getNumFunctions());
}

auto LLVMBasedICFG::Builder::buildCallGraph(Soundness /*S*/)
    -> CallGraph<n_t, f_t> {
  PHASAR_LOG_LEVEL_CAT(INFO, "LLVMBasedICFG",
                       "Starting CallGraphAnalysisType: " << Res->str());
  VisitedFunctions.reserve(IRDB->getNumFunctions());

  bool FixpointReached;

  do {
    FixpointReached = true;
    while (!FunctionWL.empty()) {
      const llvm::Function *F = FunctionWL.pop_back_val();
      FixpointReached &= processFunction(F);
    }

    /// XXX This can probably be done more efficiently.
    /// However, we cannot just work on the IndirectCalls-delta as we are
    /// mutating the points-to-info on the fly
    for (auto [CS, _] : IndirectCalls) {
      FixpointReached &= !constructDynamicCall(CS);
    }

  } while (!FixpointReached);
  for (const auto &[IndirectCall, Targets] : IndirectCalls) {
    if (Targets == 0) {
      PHASAR_LOG_LEVEL(WARNING, "No callees found for callsite "
                                    << llvmIRToString(IndirectCall));
    }
  }

  PAMM_GET_INSTANCE;
  REG_COUNTER("CG Functions", CGBuilder.viewCallGraph().getNumVertexFunctions(),
              Full);
  REG_COUNTER("CG CallSites", CGBuilder.viewCallGraph().getNumVertexCallSites(),
              Full);
  PHASAR_LOG_LEVEL_CAT(INFO, "LLVMBasedICFG",
                       "Call graph has been constructed");
  return CGBuilder.consumeCallGraph();
}

bool LLVMBasedICFG::Builder::processFunction(const llvm::Function *F) {
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                       "Walking in function: " << F->getName());
  if (F->isDeclaration() || !VisitedFunctions.insert(F).second) {
    PHASAR_LOG_LEVEL_CAT(
        DEBUG, "LLVMBasedICFG",
        "Function already visited or only declaration: " << F->getName());
    return true;
  }

  assert(Res != nullptr);

  // add a node for function F to the call graph (if not present already)
  std::ignore = CGBuilder.addFunctionVertex(F);

  bool FixpointReached = true;

  // iterate all instructions of the current function
  Resolver::FunctionSetTy PossibleTargets;
  for (const auto &I : llvm::instructions(F)) {
    if (const auto *CS = llvm::dyn_cast<llvm::CallBase>(&I)) {
      Res->preCall(&I);

      // check if function call can be resolved statically
      if (CS->getCalledFunction() != nullptr) {
        PossibleTargets.insert(CS->getCalledFunction());

        PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                             "Found static call-site: "
                                 << "  " << llvmIRToString(CS));
      } else {
        // still try to resolve the called function statically
        const llvm::Value *SV = CS->getCalledOperand()->stripPointerCasts();
        const llvm::Function *ValueFunction =
            !SV->hasName() ? nullptr : IRDB->getFunction(SV->getName());
        if (ValueFunction) {
          PossibleTargets.insert(ValueFunction);
          PHASAR_LOG_LEVEL_CAT(
              DEBUG, "LLVMBasedICFG",
              "Found static call-site: " << llvmIRToString(CS));
        } else {
          if (llvm::isa<llvm::InlineAsm>(SV)) {
            continue;
          }
          // the function call must be resolved dynamically
          PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                               "Found dynamic call-site: "
                                   << "  " << llvmIRToString(CS));
          IndirectCalls[CS] = 0;
          std::ignore = CGBuilder.addInstructionVertex(CS);

          FixpointReached = false;
          continue;
        }
      }

      PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                           "Found " << PossibleTargets.size()
                                    << " possible target(s)");

      Res->handlePossibleTargets(CS, PossibleTargets);

      auto *CallSiteId = CGBuilder.addInstructionVertex(CS);

      // Insert possible target inside the graph and add the link with
      // the current function
      for (const auto *PossibleTarget : PossibleTargets) {
        CGBuilder.addCallEdge(CS, CallSiteId, PossibleTarget);

        FunctionWL.push_back(PossibleTarget);
      }

      Res->postCall(&I);
    } else {
      Res->otherInst(&I);
    }
    PossibleTargets.clear();
  }

  return FixpointReached;
}

static bool internalIsVirtualFunctionCall(const llvm::Instruction *Inst,
                                          const LLVMTypeHierarchy &TH) {
  assert(Inst != nullptr);
  const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Inst);
  if (!CallSite) {
    return false;
  }
  // check potential receiver type
  const auto *RecType = getReceiverType(CallSite);
  if (!RecType) {
    return false;
  }
  if (!TH.hasType(RecType)) {
    return false;
  }
  if (!TH.hasVFTable(RecType)) {
    return false;
  }
  return getVFTIndex(CallSite) >= 0;
}

bool LLVMBasedICFG::Builder::constructDynamicCall(const llvm::Instruction *CS) {
  bool NewTargetsFound = false;
  // Find vertex of calling function.

  auto *Callees = CGBuilder.getInstVertexOrNull(CS);

  if (!Callees) {
    llvm::report_fatal_error(
        "constructDynamicCall: Did not find vertex of calling function " +
        CS->getFunction()->getName() + " at callsite " + llvmIRToString(CS));
  }

  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(CS)) {
    Res->preCall(CallSite);

    // the function call must be resolved dynamically
    PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                         "Looking into dynamic call-site: ");
    PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG", "  " << llvmIRToString(CS));
    // call the resolve routine

    assert(TH != nullptr);
    auto PossibleTargets = internalIsVirtualFunctionCall(CallSite, *TH)
                               ? Res->resolveVirtualCall(CallSite)
                               : Res->resolveFunctionPointer(CallSite);

    assert(IndirectCalls.count(CallSite));

    auto &NumIndCalls = IndirectCalls[CallSite];

    if (NumIndCalls < PossibleTargets.size()) {
      PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                           "Found " << PossibleTargets.size() - NumIndCalls
                                    << " new possible target(s)");
      NumIndCalls = PossibleTargets.size();
      NewTargetsFound = true;
    }
    if (!NewTargetsFound) {
      return NewTargetsFound;
    }

    // Throw out already found targets
    for (const auto *Tgt : *Callees) {
      PossibleTargets.erase(Tgt);
    }

    Res->handlePossibleTargets(CallSite, PossibleTargets);
    // Insert possible target inside the graph and add the link with
    // the current function
    for (const auto *PossibleTarget : PossibleTargets) {
      CGBuilder.addCallEdge(CallSite, Callees, PossibleTarget);
      FunctionWL.push_back(PossibleTarget);
    }

    Res->postCall(CallSite);
  } else {
    Res->otherInst(CS);
  }

  return NewTargetsFound;
}

LLVMBasedICFG::LLVMBasedICFG(LLVMProjectIRDB *IRDB,
                             CallGraphAnalysisType CGType,
                             llvm::ArrayRef<std::string> EntryPoints,
                             LLVMTypeHierarchy *TH, LLVMAliasInfoRef PT,
                             Soundness S, bool IncludeGlobals)
    : IRDB(IRDB), TH(TH) {
  assert(IRDB != nullptr);

  if (!TH) {
    this->TH = std::make_unique<LLVMTypeHierarchy>(*IRDB);
  }

  Builder B{IRDB, PT, this->TH.get()};
  LLVMAliasInfo PTOwn;

  if (!PT && CGType == CallGraphAnalysisType::OTF) {
    PTOwn = std::make_unique<LLVMAliasSet>(IRDB);
    B.PT = PTOwn.asRef();
  }

  B.Res = Resolver::create(CGType, IRDB, this->TH.get(), this, B.PT);
  B.initEntryPoints(EntryPoints);
  B.initGlobalsAndWorkList(this, IncludeGlobals);

  PHASAR_LOG_LEVEL_CAT(
      INFO, "LLVMBasedICFG",
      "Starting ICFG construction "
          << std::chrono::steady_clock::now().time_since_epoch().count());

  this->CG = B.buildCallGraph(S);

  PHASAR_LOG_LEVEL_CAT(
      INFO, "LLVMBasedICFG",
      "Finished ICFG construction "
          << std::chrono::steady_clock::now().time_since_epoch().count());
}

LLVMBasedICFG::LLVMBasedICFG(CallGraph<n_t, f_t> CG, LLVMProjectIRDB *IRDB,
                             LLVMTypeHierarchy *TH)
    : CG(std::move(CG)), IRDB(IRDB), TH(TH) {
  if (!TH) {
    this->TH = std::make_unique<LLVMTypeHierarchy>(*IRDB);
  }
}

LLVMBasedICFG::LLVMBasedICFG(LLVMProjectIRDB *IRDB,
                             const nlohmann::json &SerializedCG,
                             LLVMTypeHierarchy *TH)
    : CG(CallGraph<n_t, f_t>::deserialize(
          SerializedCG,
          [IRDB](llvm::StringRef Name) { return IRDB->getFunction(Name); },
          [IRDB](size_t Id) { return IRDB->getInstruction(Id); })),
      IRDB(IRDB) {
  if (!TH) {
    this->TH = std::make_unique<LLVMTypeHierarchy>(*IRDB);
  }
}

LLVMBasedICFG::~LLVMBasedICFG() = default;

bool LLVMBasedICFG::isPhasarGenerated(const llvm::Function &F) noexcept {
  if (F.hasName()) {
    llvm::StringRef FunctionName = F.getName();
    return llvm::StringSwitch<bool>(FunctionName)
        .Cases(GlobalCRuntimeModelName, GlobalCRuntimeDtorModelName,
               GlobalCRuntimeDtorsCallerName,
               GlobalCRuntimeUserEntrySelectorName, true)
        .Default(false);
  }

  return false;
}

[[nodiscard]] FunctionRange LLVMBasedICFG::getAllFunctionsImpl() const {
  return IRDB->getAllFunctions();
}

[[nodiscard]] auto LLVMBasedICFG::getFunctionImpl(llvm::StringRef Fun) const
    -> f_t {
  return IRDB->getFunction(Fun);
}

[[nodiscard]] bool LLVMBasedICFG::isIndirectFunctionCallImpl(n_t Inst) const {
  const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(Inst);
  return CallSite && CallSite->isIndirectCall();
}

[[nodiscard]] bool LLVMBasedICFG::isVirtualFunctionCallImpl(n_t Inst) const {
  return internalIsVirtualFunctionCall(Inst, *TH);
}

[[nodiscard]] auto LLVMBasedICFG::allNonCallStartNodesImpl() const
    -> std::vector<n_t> {
  std::vector<n_t> NonCallStartNodes;
  NonCallStartNodes.reserve(2 * IRDB->getNumFunctions());
  for (const auto *Inst : IRDB->getAllInstructions()) {
    if (!llvm::isa<llvm::CallBase>(Inst) && !isStartPoint(Inst)) {
      NonCallStartNodes.push_back(Inst);
    }
  }

  return NonCallStartNodes;
}

[[nodiscard]] auto LLVMBasedICFG::getCallsFromWithinImpl(f_t Fun) const
    -> llvm::SmallVector<n_t> {
  llvm::SmallVector<n_t> CallSites;
  for (const auto &I : llvm::instructions(Fun)) {
    if (llvm::isa<llvm::CallBase>(I)) {
      CallSites.push_back(&I);
    }
  }
  return CallSites;
}

[[nodiscard]] auto LLVMBasedICFG::getReturnSitesOfCallAtImpl(n_t Inst) const
    -> llvm::SmallVector<n_t, 2> {
  /// Currently, we don't distinguish normal-dest and unwind-dest, so we can
  /// just use getSuccsOf

  return getSuccsOf(Inst);
}

void LLVMBasedICFG::printImpl(llvm::raw_ostream &OS) const {
  CG.printAsDot(
      OS, [](f_t Fun) { return Fun->getName(); },
      [](n_t CS) { return CS->getFunction(); },
      [](n_t CS) { return llvmIRToStableString(CS); });
}

[[nodiscard]] nlohmann::json LLVMBasedICFG::getAsJsonImpl() const {
  return CG.getAsJson(
      [](f_t F) { return F->getName().str(); },
      [this](n_t Inst) { return IRDB->getInstructionId(Inst); });
}

} // namespace psr
