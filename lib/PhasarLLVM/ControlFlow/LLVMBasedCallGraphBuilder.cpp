#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Soundness.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"

#include <memory>

namespace {
using namespace psr;
struct Builder {
  LLVMProjectIRDB *IRDB = nullptr;
  LLVMTypeHierarchy *TH{};
  Resolver *Res = nullptr;
  CallGraphBuilder<const llvm::Instruction *, const llvm::Function *>
      CGBuilder{};
  llvm::DenseSet<const llvm::Function *> VisitedFunctions{};

  llvm::Function *GlobalCleanupFn = nullptr;

  llvm::SmallDenseMap<const llvm::Module *, llvm::Function *>
      GlobalRegisteredDtorsCaller{};

  // The worklist for direct callee resolution.
  llvm::SmallVector<const llvm::Function *, 0> FunctionWL{};

  // Map indirect calls to the number of possible targets found for it. Fixpoint
  // is not reached when more targets are found.
  llvm::DenseMap<const llvm::Instruction *, unsigned> IndirectCalls{};

  void initWorkList(llvm::ArrayRef<const llvm::Function *> EntryPointFns);

  [[nodiscard]] CallGraph<const llvm::Instruction *, const llvm::Function *>
  buildCallGraph(Soundness S);

  /// \returns FixPointReached
  bool processFunction(/*bidigraph_t &Callgraph,*/ const llvm::Function *F);
  /// \returns FoundNewTargets
  bool constructDynamicCall(const llvm::Instruction *CS);
};

void Builder::initWorkList(
    llvm::ArrayRef<const llvm::Function *> EntryPointFns) {
  FunctionWL.reserve(IRDB->getNumFunctions());
  FunctionWL.append(EntryPointFns.begin(), EntryPointFns.end());

  CGBuilder.reserve(IRDB->getNumFunctions());
}

auto Builder::buildCallGraph(Soundness S) -> LLVMBasedCallGraph {
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

    if (S != psr::Soundness::Unsound) {
      /// XXX This can probably be done more efficiently.
      /// However, we cannot just work on the IndirectCalls-delta as we are
      /// mutating the points-to-info on the fly
      for (auto [CS, _] : IndirectCalls) {
        FixpointReached &= !constructDynamicCall(CS);
      }
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

bool Builder::processFunction(const llvm::Function *F) {
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
        const llvm::Value *SV =
            CS->getCalledOperand()->stripPointerCastsAndAliases();
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

          assert(TH != nullptr);
          PossibleTargets = internalIsVirtualFunctionCall(CS, *TH)
                                ? Res->resolveVirtualCall(CS)
                                : Res->resolveFunctionPointer(CS);

          IndirectCalls[CS] = PossibleTargets.size();
          std::ignore = CGBuilder.addInstructionVertex(CS);

          FixpointReached = false;
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

bool Builder::constructDynamicCall(const llvm::Instruction *CS) {
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
} // namespace

auto psr::buildLLVMBasedCallGraph(
    LLVMProjectIRDB &IRDB, Resolver &CGResolver,
    llvm::ArrayRef<const llvm::Function *> EntryPoints, LLVMTypeHierarchy &TH,
    Soundness S) -> LLVMBasedCallGraph {
  Builder B{&IRDB, &TH, &CGResolver};

  B.initWorkList(EntryPoints);

  PHASAR_LOG_LEVEL_CAT(
      INFO, "LLVMBasedICFG",
      "Starting ICFG construction "
          << std::chrono::steady_clock::now().time_since_epoch().count());

  scope_exit FinishTiming = [] {
    PHASAR_LOG_LEVEL_CAT(
        INFO, "LLVMBasedICFG",
        "Finished ICFG construction "
            << std::chrono::steady_clock::now().time_since_epoch().count());
  };

  return B.buildCallGraph(S);
}

auto psr::buildLLVMBasedCallGraph(
    LLVMProjectIRDB &IRDB, CallGraphAnalysisType CGType,
    llvm::ArrayRef<const llvm::Function *> EntryPoints, LLVMTypeHierarchy &TH,
    LLVMAliasInfoRef PT, Soundness S) -> LLVMBasedCallGraph {

  LLVMAliasInfo PTOwn;
  if (!PT && CGType == CallGraphAnalysisType::OTF) {
    PTOwn = std::make_unique<LLVMAliasSet>(&IRDB);
    PT = PTOwn.asRef();
  }

  auto Res = Resolver::create(CGType, &IRDB, &TH);
  return buildLLVMBasedCallGraph(IRDB, *Res, EntryPoints, TH, S);
}

auto psr::buildLLVMBasedCallGraph(LLVMProjectIRDB &IRDB,
                                  CallGraphAnalysisType CGType,
                                  llvm::ArrayRef<std::string> EntryPoints,
                                  LLVMTypeHierarchy &TH, LLVMAliasInfoRef PT,
                                  Soundness S) -> LLVMBasedCallGraph {
  auto EntryPointFns = getEntryFunctions(IRDB, EntryPoints);
  return buildLLVMBasedCallGraph(IRDB, CGType, EntryPointFns, TH, PT, S);
}

auto psr::buildLLVMBasedCallGraph(LLVMProjectIRDB &IRDB, Resolver &CGResolver,
                                  llvm::ArrayRef<std::string> EntryPoints,
                                  LLVMTypeHierarchy &TH, Soundness S)
    -> LLVMBasedCallGraph {
  auto EntryPointFns = getEntryFunctions(IRDB, EntryPoints);
  return buildLLVMBasedCallGraph(IRDB, CGResolver, EntryPointFns, TH, S);
}
