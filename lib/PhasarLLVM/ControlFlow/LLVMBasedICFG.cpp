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
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/CFGBase.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
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
#include "llvm/Support/ErrorHandling.h"

#include <cstdint>
#include <utility>

namespace psr {
struct LLVMBasedICFG::Builder {
  ProjectIRDB *IRDB = nullptr;
  LLVMBasedICFG *ICF = nullptr;
  LLVMAliasInfoRef PT{};
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
  void buildCallGraph(Soundness S);

  /// \returns FixPointReached
  bool processFunction(/*bidigraph_t &Callgraph,*/ const llvm::Function *F);
  /// \returns FoundNewTargets
  bool constructDynamicCall(const llvm::Instruction *CS);
};

void LLVMBasedICFG::Builder::initEntryPoints(
    llvm::ArrayRef<std::string> EntryPoints) {
  if (EntryPoints.size() == 1 && EntryPoints.front() == "__ALL__") {
    // Handle the special case in which a user wishes to treat all functions as
    // entry points.
    for (const auto *Fun : IRDB->getAllFunctions()) {
      if (!Fun->isDeclaration() && Fun->hasName()) {
        UserEntryPoints.push_back(IRDB->getFunctionDefinition(Fun->getName()));
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
  FunctionWL.reserve(IRDB->getAllFunctions().size());
  if (IncludeGlobals) {
    assert(IRDB->getNumberOfModules() == 1 &&
           "IncludeGlobals is currently only supported for WPA");
    const auto *GlobCtor = ICFG->buildCRuntimeGlobalCtorsDtorsModel(
        *IRDB->getWPAModule(), UserEntryPoints);
    FunctionWL.push_back(GlobCtor);
  } else {
    FunctionWL.insert(FunctionWL.end(), UserEntryPoints.begin(),
                      UserEntryPoints.end());
  }
}

void LLVMBasedICFG::Builder::buildCallGraph(Soundness /*S*/) {
  PHASAR_LOG_LEVEL_CAT(INFO, "LLVMBasedICFG",
                       "Starting CallGraphAnalysisType: " << Res->str());
  VisitedFunctions.reserve(IRDB->getAllFunctions().size());

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
  REG_COUNTER("CG Vertices", boost::num_vertices(ret),
              PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("CG Edges", boost::num_edges(ret), PAMM_SEVERITY_LEVEL::Full);
  PHASAR_LOG_LEVEL_CAT(INFO, "LLVMBasedICFG",
                       "Call graph has been constructed");
  // return Ret;
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
  ICF->addFunctionVertex(F);

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
          ICF->addInstructionVertex(CS);

          FixpointReached = false;
          continue;
        }
      }

      PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                           "Found " << PossibleTargets.size()
                                    << " possible target(s)");

      Res->handlePossibleTargets(CS, PossibleTargets);

      auto *CallSiteId = ICF->addInstructionVertex(CS);

      // Insert possible target inside the graph and add the link with
      // the current function
      for (const auto *PossibleTarget : PossibleTargets) {
        ICF->addCallEdge(CS, CallSiteId, PossibleTarget);

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

llvm::SmallVector<const llvm::Instruction *> *
LLVMBasedICFG::addFunctionVertex(const llvm::Function *F) {
  auto [It, Inserted] = CallersOf.try_emplace(F, nullptr);
  if (Inserted) {
    VertexFunctions.push_back(F);

    using type = llvm::SmallVector<const llvm::Instruction *>;
    auto *RawBytes =
#if HAS_MEMORY_RESOURCE
        MRes.allocate(sizeof(type), alignof(type));
#else
        MRes.Allocate<type>();
#endif
    It->second.reset(new (RawBytes) type());
  }

  return It->second.get();
}

llvm::SmallVector<const llvm::Function *> *
LLVMBasedICFG::addInstructionVertex(const llvm::Instruction *Inst) {
  auto [It, Inserted] = CalleesAt.try_emplace(Inst, nullptr);
  if (Inserted) {
    using type = llvm::SmallVector<const llvm::Function *>;
    auto *RawBytes =
#if HAS_MEMORY_RESOURCE
        MRes.allocate(sizeof(type), alignof(type));
#else
        MRes.Allocate<type>();
#endif
    It->second.reset(new (RawBytes) type());
  }

  return It->second.get();
}

void LLVMBasedICFG::addCallEdge(const llvm::Instruction *CS,
                                const llvm::Function *Callee) {
  return addCallEdge(CS, addInstructionVertex(CS), Callee);
}

void LLVMBasedICFG::addCallEdge(
    const llvm::Instruction *CS,
    llvm::SmallVector<const llvm::Function *> *Callees,
    const llvm::Function *Callee) {

  auto *Callers = addFunctionVertex(Callee);

  Callees->push_back(Callee);
  Callers->push_back(CS);
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

  auto FvmItr = ICF->CalleesAt.find(CS);

  if (FvmItr == ICF->CalleesAt.end()) {
    llvm::report_fatal_error(
        "constructDynamicCall: Did not find vertex of calling function " +
        CS->getFunction()->getName() + " at callsite " + llvmIRToString(CS));
  }

  auto *Callees = FvmItr->second.get();

  if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(CS)) {
    Res->preCall(CallSite);

    // the function call must be resolved dynamically
    PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                         "Looking into dynamic call-site: ");
    PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG", "  " << llvmIRToString(CS));
    // call the resolve routine

    assert(ICF->TH != nullptr);
    auto PossibleTargets = internalIsVirtualFunctionCall(CallSite, *ICF->TH)
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
      ICF->addCallEdge(CallSite, Callees, PossibleTarget);
      FunctionWL.push_back(PossibleTarget);
    }

    Res->postCall(CallSite);
  } else {
    Res->otherInst(CS);
  }

  return NewTargetsFound;
}

LLVMBasedICFG::LLVMBasedICFG(ProjectIRDB *IRDB, CallGraphAnalysisType CGType,
                             llvm::ArrayRef<std::string> EntryPoints,
                             LLVMTypeHierarchy *TH, LLVMAliasInfoRef PT,
                             Soundness S, bool IncludeGlobals)
    : TH(TH) {
  assert(IRDB != nullptr);
  this->IRDB = IRDB;

  Builder B{IRDB, this, PT};
  LLVMAliasInfo PTOwn;

  if (!TH && CGType != CallGraphAnalysisType::NORESOLVE) {
    this->TH = std::make_unique<LLVMTypeHierarchy>(*IRDB);
  }
  if (!PT && CGType == CallGraphAnalysisType::OTF) {
    PTOwn = std::make_unique<LLVMAliasSet>(*IRDB);
    B.PT = PTOwn.asRef();
  }

  B.Res = Resolver::create(CGType, IRDB, this->TH.get(), this, B.PT);
  B.initEntryPoints(EntryPoints);
  B.initGlobalsAndWorkList(this, IncludeGlobals);

  PHASAR_LOG_LEVEL_CAT(
      INFO, "LLVMBasedICFG",
      "Starting ICFG construction "
          << std::chrono::steady_clock::now().time_since_epoch().count());

  B.buildCallGraph(S);

  PHASAR_LOG_LEVEL_CAT(
      INFO, "LLVMBasedICFG",
      "Finished ICFG construction "
          << std::chrono::steady_clock::now().time_since_epoch().count());
}

LLVMBasedICFG::~LLVMBasedICFG() = default;

[[nodiscard]] FunctionRange LLVMBasedICFG::getAllFunctionsImpl() const {
  /// With the new LLVMProjectIRDB, this will be easier...
  return llvm::map_range(
      static_cast<const llvm::Module &>(*IRDB->getWPAModule()),
      Ref2PointerConverter<llvm::Function>{});
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
  /// NOTE: Gets more performant once we have the new LLVMProjectIRDB
  NonCallStartNodes.reserve(2 * IRDB->getAllFunctions().size());
  for (const auto *F : IRDB->getAllFunctions()) {
    for (const auto &I : llvm::instructions(F)) {
      if (!llvm::isa<llvm::CallBase>(&I) && !isStartPoint(&I)) {
        NonCallStartNodes.push_back(&I);
      }
    }
  }
  return NonCallStartNodes;
}

[[nodiscard]] auto
LLVMBasedICFG::getCalleesOfCallAtImpl(n_t Inst) const noexcept
    -> llvm::ArrayRef<f_t> {
  if (!llvm::isa<llvm::CallBase>(Inst)) {
    return {};
  }

  auto MapEntry = CalleesAt.find(Inst);
  if (MapEntry == CalleesAt.end()) {
    return {};
  }

  return *MapEntry->second;
}

[[nodiscard]] auto LLVMBasedICFG::getCallersOfImpl(f_t Fun) const noexcept
    -> llvm::ArrayRef<n_t> {
  auto MapEntry = CallersOf.find(Fun);
  if (MapEntry == CallersOf.end()) {
    return {};
  }

  return *MapEntry->second;
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
  OS << "digraph CallGraph{\n";
  scope_exit CloseBrace = [&OS] { OS << "}\n"; };

  for (const auto *Fun : VertexFunctions) {
    OS << uintptr_t(Fun) << "[label=\"";
    OS.write_escaped(Fun->getName());
    OS << "\"];\n";
    for (const auto &Inst : llvm::instructions(Fun)) {
      if (!llvm::isa<llvm::CallBase>(Inst)) {
        continue;
      }

      if (auto It = CalleesAt.find(&Inst); It != CalleesAt.end()) {
        for (const auto *Succ : *It->second) {
          assert(CallersOf.count(Succ));
          OS << uintptr_t(Fun) << "->" << uintptr_t(Succ) << "[label=\"";
          OS.write_escaped(llvmIRToStableString(&Inst));
          OS << "\"]\n;";
        }
      }
    }
    OS << '\n';
  }
}

[[nodiscard]] nlohmann::json LLVMBasedICFG::getAsJsonImpl() const {
  nlohmann::json J;

  for (size_t Vtx = 0, VtxEnd = VertexFunctions.size(); Vtx != VtxEnd; ++Vtx) {
    auto VtxFunName = VertexFunctions[Vtx]->getName().str();
    J[PhasarConfig::JsonCallGraphID()][VtxFunName] = nlohmann::json::array();

    for (const auto &Inst : llvm::instructions(VertexFunctions[Vtx])) {
      if (!llvm::isa<llvm::CallBase>(Inst)) {
        continue;
      }

      if (auto It = CalleesAt.find(&Inst); It != CalleesAt.end()) {
        for (const auto *Succ : *It->second) {
          J[PhasarConfig::JsonCallGraphID()][VtxFunName].push_back(
              Succ->getName().str());
        }
      }
    }
  }

  return J;
}

auto LLVMBasedICFG::getAllVertexFunctions() const noexcept
    -> llvm::ArrayRef<f_t> {
  return VertexFunctions;
}

} // namespace psr
