#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Soundness.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include <memory>

namespace {
using namespace psr;
struct Builder {
  const LLVMProjectIRDB *IRDB = nullptr;
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

  bool RequiresIndirectCallsFixpoint =
      S != psr::Soundness::Unsound && Res->mutatesHelperAnalysisInformation();

  bool FixpointReached;

  do {
    FixpointReached = true;
    while (!FunctionWL.empty()) {
      const llvm::Function *F = FunctionWL.pop_back_val();
      FixpointReached &= processFunction(F);
    }

    if (RequiresIndirectCallsFixpoint) {
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

static bool fillPossibleTargets(
    Resolver::FunctionSetTy &PossibleTargets, Resolver &Res,
    const llvm::CallBase *CS,
    llvm::DenseMap<const llvm::Instruction *, unsigned int> &IndirectCalls) {
  if (const auto *StaticCallee = CS->getCalledFunction()) {
    PossibleTargets.insert(StaticCallee);

    PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                         "Found static call-site: "
                             << "  " << llvmIRToString(CS));
    return true;
  }

  // still try to resolve the called function statically
  const llvm::Value *SV = CS->getCalledOperand()->stripPointerCastsAndAliases();
  if (const auto *ValueFunction = llvm::dyn_cast<llvm::Function>(SV)) {
    PossibleTargets.insert(ValueFunction);
    PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                         "Found static call-site: " << llvmIRToString(CS));
    return true;
  }

  if (llvm::isa<llvm::InlineAsm>(SV)) {
    return true;
  }

  // the function call must be resolved dynamically
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                       "Found dynamic call-site: "
                           << "  " << llvmIRToString(CS));

  PossibleTargets = Res.resolveIndirectCall(CS);

  IndirectCalls[CS] = PossibleTargets.size();
  return false;
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
    const auto *CS = llvm::dyn_cast<llvm::CallBase>(&I);
    if (!CS) {
      Res->otherInst(&I);
      continue;
    }

    Res->preCall(&I);
    scope_exit PostCall = [&] { Res->postCall(&I); };

    FixpointReached &=
        fillPossibleTargets(PossibleTargets, *Res, CS, IndirectCalls);

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
    PossibleTargets.clear();
  }

  return FixpointReached;
}

bool Builder::constructDynamicCall(const llvm::Instruction *CS) {
  const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(CS);
  if (!CallSite) {
    llvm::report_fatal_error("[constructDynamicCall]: No call: " +
                             llvm::Twine(llvmIRToString(CS)));
  }

  // Find vertex of callsite.
  auto *Callees = CGBuilder.getInstVertexOrNull(CS);
  if (!Callees) {
    llvm::report_fatal_error(
        "[constructDynamicCall]: Did not find vertex of callsite " +
        llvm::Twine(llvmIRToString(CS)));
  }

  // the function call must be resolved dynamically
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                       "Looking into dynamic call-site: ");
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG", "  " << llvmIRToString(CS));

  Res->preCall(CallSite);
  scope_exit PostCall = [&] { Res->postCall(CallSite); };

  // call the resolve routine

  auto PossibleTargets = Res->resolveIndirectCall(CallSite);

  assert(IndirectCalls.count(CallSite));
  auto &NumIndCalls = IndirectCalls[CallSite];

  if (NumIndCalls >= PossibleTargets.size()) {
    // No new targets found
    return false;
  }

  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMBasedICFG",
                       "Found " << PossibleTargets.size() - NumIndCalls
                                << " new possible target(s)");
  NumIndCalls = PossibleTargets.size();

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

  return true;
}
} // namespace

auto psr::buildLLVMBasedCallGraph(
    const LLVMProjectIRDB &IRDB, Resolver &CGResolver,
    llvm::ArrayRef<const llvm::Function *> EntryPoints, Soundness S)
    -> LLVMBasedCallGraph {
  Builder B{&IRDB, &CGResolver};

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
    llvm::ArrayRef<const llvm::Function *> EntryPoints,
    DIBasedTypeHierarchy &TH, LLVMVFTableProvider &VTP, LLVMAliasInfoRef PT,
    Soundness S) -> LLVMBasedCallGraph {

  LLVMAliasInfo PTOwn;
  if (!PT && CGType == CallGraphAnalysisType::OTF) {
    PTOwn = std::make_unique<LLVMAliasSet>(&IRDB);
    PT = PTOwn.asRef();
  }

  auto Res = Resolver::create(CGType, &IRDB, &VTP, &TH);
  return buildLLVMBasedCallGraph(IRDB, *Res, EntryPoints, S);
}

auto psr::buildLLVMBasedCallGraph(LLVMProjectIRDB &IRDB,
                                  CallGraphAnalysisType CGType,
                                  llvm::ArrayRef<std::string> EntryPoints,
                                  DIBasedTypeHierarchy &TH,
                                  LLVMVFTableProvider &VTP, LLVMAliasInfoRef PT,
                                  Soundness S) -> LLVMBasedCallGraph {
  auto EntryPointFns = getEntryFunctions(IRDB, EntryPoints);
  return buildLLVMBasedCallGraph(IRDB, CGType, EntryPointFns, TH, VTP, PT, S);
}

auto psr::buildLLVMBasedCallGraph(const LLVMProjectIRDB &IRDB,
                                  Resolver &CGResolver,
                                  llvm::ArrayRef<std::string> EntryPoints,
                                  Soundness S) -> LLVMBasedCallGraph {
  auto EntryPointFns = getEntryFunctions(IRDB, EntryPoints);
  return buildLLVMBasedCallGraph(IRDB, CGResolver, EntryPointFns, S);
}
