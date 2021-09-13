/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/ADT/SmallSet.h"
#include <algorithm>
#include <type_traits>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/GenEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/Helpers.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/JoinEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/KillIfSanitizedEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/TransferEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/DebugOutput.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

namespace psr::XTaint {
// Misc:

template <typename ContainerTy>
inline void printValues(const ContainerTy &Facts,
                        std::ostream &OS = std::cerr) {
  OS << "{";

  for (const auto *Fact : Facts) {
    OS << "\n\t" << llvmIRToString(Fact);
  }

  if (!Facts.empty()) {
    OS << "\n";
  }
  OS << "}";
}

IDEExtendedTaintAnalysis::IDEExtendedTaintAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT, const TaintConfig *TSF,
    std::set<std::string> EntryPoints, unsigned bound,
    bool disableStrongUpdates)
    : base_t(IRDB, TH, ICF, PT, std::move(EntryPoints)), AnalysisBase(TSF),
      FactFactory(IRDB->getNumInstructions()),
      DL((*IRDB->getAllModules().begin())->getDataLayout()), bound(bound),
      postProcessed(disableStrongUpdates),
      disableStrongUpdates(disableStrongUpdates) {
  base_t::ZeroValue = createZeroValue();

  FactFactory.setDataLayout(DL);
}

InitialSeeds<IDEExtendedTaintAnalysis::n_t, IDEExtendedTaintAnalysis::d_t,
             IDEExtendedTaintAnalysis::l_t>
IDEExtendedTaintAnalysis::initialSeeds() {
  InitialSeeds<IDEExtendedTaintAnalysis::n_t, IDEExtendedTaintAnalysis::d_t,
               IDEExtendedTaintAnalysis::l_t>
      Seeds;
  auto AutoSeeds = TSF->makeInitialSeeds();

  for (auto &[Inst, Facts] : AutoSeeds) {
    for (const auto *Fact : Facts) {
      Seeds.addSeed(Inst, makeFlowFact(Fact), nullptr);
    }
  }

  for (const auto &Ep : base_t::EntryPoints) {
    const auto *EntryFn = base_t::ICF->getFunction(Ep);

    if (!EntryFn) {
      std::cerr << "WARNING: Entry-Function \"" << Ep
                << "\" not contained in the module; skip it\n";
      continue;
    }

    Seeds.addSeed(&EntryFn->front().front(), this->base_t::getZeroValue(),
                  bottomElement());
  }

  return Seeds;
}

auto IDEExtendedTaintAnalysis::createZeroValue() const -> d_t {
  return FactFactory.GetOrCreateZero();
}

bool IDEExtendedTaintAnalysis::isZeroValue(d_t d) const { return d->isZero(); }

IDEExtendedTaintAnalysis::EdgeFunctionPtrType
IDEExtendedTaintAnalysis::allTopFunction() {
  return getAllTop();
}

// Flow functions:

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getNormalFlowFunction(n_t curr, n_t succ) {

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "##Normal-FF at: " << psr::llvmIRToString(curr));

  // The only instruction we need to handle in the Normal-FF is the StoreInst.
  // All other instructions are handled by the recursive Create function from
  // the FactFactory.

  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    auto storeFF =
        getStoreFF(Store->getPointerOperand(), Store->getValueOperand(), Store);

    return storeFF;
  }

  auto [SrcConfig, SinkConfig] = getConfigurationAt(curr);
  if (!SrcConfig.empty() || !SinkConfig.empty()) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "handle config in normal-ff");
    return handleConfig(curr, std::move(SrcConfig), std::move(SinkConfig));
  }

  return Identity<d_t>::getInstance();
}

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getStoreFF(const llvm::Value *PointerOp,
                                     const llvm::Value *ValueOp,
                                     const llvm::Instruction *Store) {

  auto TV = makeFlowFact(ValueOp);
  auto PTS = this->PT->getPointsToSet(PointerOp, Store);

  auto Mem = makeFlowFact(PointerOp);
  return makeLambdaFlow<d_t>([this, TV, Mem, PTS{std::move(PTS)}, PointerOp,
                              ValueOp,
                              Store](d_t source) mutable -> std::set<d_t> {
    if (source->isZero()) {
      std::set<d_t> ret = {source};
      generateFromZero(ret, Store, PointerOp, ValueOp,
                       /*IncludeActualArg*/ false);
      return ret;
    }
    /// Pointer-Arithetics in the last indirection are irrelevant for equality
    /// comparison:
    /// For example: Let source be a pointer to a tainted array-element and TV
    /// be a pointer to a different element of the same array. Then the taint is
    /// easily reachable from TV by simply doing dome pointer arithmetics.
    /// Hence, when loading the value of TV back from Mem this still holds and
    /// must be preserved by the analysis.
    if (TV->equivalentExceptPointerArithmetics(source)) {
      auto offset = source - TV;

      // generate all may-aliases of Store->getPointerOperand()
      std::set<d_t> ret = {source, FactFactory.withIndirectionOf(Mem, offset)};

      for (const auto *Alias : *PTS) {
        if (llvm::isa<llvm::Constant>(Alias) || Alias == Mem->base()) {
          continue;
        }

        identity(ret,
                 FactFactory.withIndirectionOf(makeFlowFact(Alias), offset),
                 Store);
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Store generate: " << SequencePrinter(ret));

      // For the sink-variables, the pointer-arithmetics in the last offset
      // are relevant (in contrast to the Store-FF). This is, where the
      // field-sensitive results come from.
      if (TV->equivalent(source)) {
        reportLeakIfNecessary(Store, PointerOp, ValueOp);
        for (const auto *Alias : *PTS) {
          reportLeakIfNecessary(Store, Alias, ValueOp);
        }
      }

#ifdef XTAINT_DIAGNOSTICS
      allTaintedValues.insert(ret.begin(), ret.end());
#endif

      return ret;
    }
    // Sanitizing is handled in the edge function

    return {source};
  });
}

void IDEExtendedTaintAnalysis::generateFromZero(std::set<d_t> &Dest,
                                                const llvm::Instruction *Inst,
                                                const llvm::Value *FormalArg,
                                                const llvm::Value *ActualArg,
                                                bool IncludeActualArg) {
  config_callback_t SourceCB;

  if (TSF->isSource(ActualArg) ||
      ((SourceCB = TSF->getRegisteredSourceCallBack()) &&
       SourceCB(Inst).count(ActualArg))) {
    Dest.insert(makeFlowFact(FormalArg));
    if (IncludeActualArg) {
      Dest.insert(makeFlowFact(ActualArg));
    }
  }
}
void IDEExtendedTaintAnalysis::reportLeakIfNecessary(
    const llvm::Instruction *Inst, const llvm::Value *SinkCandidate,
    const llvm::Value *LeakCandidate) {
  if (const auto &SinkCB = TSF->getRegisteredSinkCallBack();
      TSF->isSink(SinkCandidate) ||
      (SinkCB && SinkCB(Inst).count(SinkCandidate))) {
    Leaks[Inst].insert(LeakCandidate);
  }
}

void IDEExtendedTaintAnalysis::populateWithMayAliases(
    SourceConfigTy &Facts) const {

  SourceConfigTy Tmp = Facts;
  for (const auto *Fact : Facts) {
    auto Aliases = PT->getPointsToSet(Fact);

    Tmp.insert(Aliases->begin(), Aliases->end());
  }

  Facts = std::move(Tmp);
}

bool IDEExtendedTaintAnalysis::isMustAlias(const SanitizerConfigTy &Facts,
                                           d_t CurrNod) {

  return std::any_of(Facts.begin(), Facts.end(),
                     [this, CurrNod](const auto *Fact) {
                       return makeFlowFact(Fact)->equivalent(CurrNod);
                     });
}

auto IDEExtendedTaintAnalysis::handleConfig(const llvm::Instruction *Inst,
                                            SourceConfigTy &&SourceConfig,
                                            SinkConfigTy &&SinkConfig)
    -> FlowFunctionPtrType {

#ifdef XTAINT_DIAGNOSTICS
  allTaintedValues.insert(SourceConfig.begin(), SourceConfig.end());
#endif

  /// TODO: Once the points-to analysis is rudimentarily precise, remove the
  /// comment and generate aliases
  // populateWithMayAliases(SourceConfig);

  return makeLambdaFlow<d_t>([Inst, this, SourceConfig{std::move(SourceConfig)},
                              SinkConfig{std::move(SinkConfig)}](d_t Source) {
    std::set<d_t> ret = {Source};

    if (Source->isZero()) {
      for (const auto *Src : SourceConfig) {
        ret.insert(makeFlowFact(Src));
      }
    } else {
      for (const auto *Snk : SinkConfig) {
        if (equivalent(Source, makeFlowFact(Snk))) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Leaking: " << llvmIRToString(Snk));
          Leaks[Inst].insert(Snk);
        }
      }
    }

    return ret;
  });
}

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getCallFlowFunction(n_t callStmt, f_t destFun) {
  const auto *call = llvm::cast<llvm::CallBase>(callStmt);
  assert(call);
  if (destFun->isDeclaration()) {
    return Identity<d_t>::getInstance();
  }
  bool hasVarargs = call->arg_size() > destFun->arg_size();
  const auto *const va = [&]() -> const llvm::Value * {
    if (!hasVarargs) {
      return nullptr;
    }
    // Copied from IDELinearConstantAnalysis:
    // Over-approximate by trying to add the
    //   alloca [1 x %struct.__va_list_tag], align 16
    // to the results
    // find the allocated %struct.__va_list_tag and generate it
    for (auto it = llvm::inst_begin(destFun), end = llvm::inst_end(destFun);
         it != end; ++it) {
      if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(&*it)) {
        if (Alloc->getAllocatedType()->isArrayTy() &&
            Alloc->getAllocatedType()->getArrayNumElements() > 0 &&
            Alloc->getAllocatedType()->getArrayElementType()->isStructTy() &&
            Alloc->getAllocatedType()->getArrayElementType()->getStructName() ==
                "struct.__va_list_tag") {
          return Alloc;
        }
      }
    }
    // Maybe the va_list is unused in the function body
    return nullptr;
  }();

  return makeLambdaFlow<d_t>(
      [this, call, destFun, va](d_t source) -> std::set<d_t> {
        if (isZeroValue(source)) {
          return {source};
        }
        std::set<d_t> ret;
        /// Don't qualify the 'auto' here, because we should not rely on those
        /// iterators to be pointers

        // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
        auto it = call->arg_begin();
        // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
        auto end = call->arg_end();
        // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
        auto fit = destFun->arg_begin();
        // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
        auto fend = destFun->arg_end();

        const std::string CalleeName =
            (destFun->hasName()) ? destFun->getName().str() : "none";
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "##Call-FF at: " << psr::llvmIRToString(call)
                      << " to: " << CalleeName);
        for (; fit != fend && it != end; ++fit, ++it) {
          auto from = makeFlowFact(it->get());
          /// Pointer-Arithetics in the last indirection are irrelevant for
          /// equality comparison. Argumentation similar to StoreFF
          if (equivalentExceptPointerArithmetics(from, source)) {
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << ">\tmatch: " << from << " vs " << source);
            ret.insert(transferFlowFact(source, from, &*fit));
          } else {
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << ">\tno match: " << from << " vs " << source);
          }
        }
        ptrdiff_t offs = 0;

        if (!va)
          return ret;

        for (; it != end; ++it) {
          auto from = makeFlowFact(it->get());
          if (equivalentExceptPointerArithmetics(from, source)) {
            auto to = transferFlowFact(source, from, va);

            ret.insert(FactFactory.withIndirectionOf(to, {offs}));
            /// Model varargs as an additional aggregate-parameter. The varargs
            /// are stored in contiguous memory one after the other. Ignore
            /// padding for now.
          }
          offs += DL.getTypeAllocSize(it->get()->getType()).getFixedSize();
        }
#ifdef XTAINT_DIAGNOSTICS
        allTaintedValues.insert(ret.begin(), ret.end());
#endif
        return ret;
      });
}

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getRetFlowFunction(n_t callSite, f_t calleeFun,
                                             n_t exitStmt, n_t retSite) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "##Return-FF at: " << psr::llvmIRToString(callSite));
  const auto *call = llvm::cast<llvm::CallBase>(callSite);
  return makeLambdaFlow<d_t>([this, call, calleeFun,
                              exitStmt{llvm::cast<llvm::ReturnInst>(exitStmt)}](
                                 d_t source) {
    std::set<d_t> ret;

    /// Don't qualify the 'auto' here, because we should not rely on those
    /// iterators to be pointers

    // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
    auto it = call->arg_begin();
    // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
    auto end = call->arg_end();
    // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
    auto fit = calleeFun->arg_begin();
    // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
    auto fend = calleeFun->arg_end();

    for (; fit != fend && it != end; ++fit, ++it) {
      // Only map back pointer parameters, since for all others we have
      // call-by-value.
      if (fit->getType()->isPointerTy() &&
          equivalent(source, makeFlowFact(&*fit))) {
        ret.insert(
            FactFactory.withTransferFrom(source, makeFlowFact(it->get())));
      }
    }
    // For now, ignore mapping back varargs
    d_t retVal;
    if (exitStmt->getReturnValue() &&
        equivalent(source, retVal = makeFlowFact(exitStmt->getReturnValue()))) {
      ret.insert(FactFactory.withOffsets(makeFlowFact(call), source - retVal));
    }

#ifdef XTAINT_DIAGNOSTICS
    allTaintedValues.insert(ret.begin(), ret.end());
#endif

    return ret;
  });
}

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getCallToRetFlowFunction(n_t callSite, n_t retSite,
                                                   std::set<f_t> callees) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "##CallToReturn-FF at: " << psr::llvmIRToString(callSite));
  // The CTR-FF is traditionally an identity function. All CTR-relevant stuff is
  // handled on the edges.

  return Identity<d_t>::getInstance();
}

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getSummaryFlowFunction(n_t callStmt, f_t destFun) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "##Summary-FF at: " << psr::llvmIRToString(callStmt));
  // Handle all the functions that have a special semantics inside the analysis:
  // - Calls to the DevAPI
  // - Calls to sources, sinks and sanitizers
  // If the seeds were autogenerated, we can ignore calls to the DevAPI here,
  // since they were already considered when constructing the seeds

  auto [SrcConfig, SinkConfig] = getConfigurationAt(callStmt, destFun);
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "SrcIndices.any(): " << !SrcConfig.empty()
                << " - SinkIndices.any(): " << !SinkConfig.empty());
  if (!SrcConfig.empty() || !SinkConfig.empty()) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "handle config in summary-ff");
    return handleConfig(callStmt, std::move(SrcConfig), std::move(SinkConfig));
  }

  /// TODO: MemSet
  //   ...
  //   } else if (auto MemSet = llvm::dyn_cast<llvm::MemSetInst>(call)) {
  //     // Basically, MemSet is the same as Store
  //     return getStoreFF(MemSet->getDest(), MemSet->getValue(), MemSet);
  //   }

  return nullptr;
}

// Edge Functions:

auto IDEExtendedTaintAnalysis::getNormalEdgeFunction(n_t Curr, d_t CurrNode,
                                                     n_t Succ, d_t SuccNode)
    -> EdgeFunctionPtrType {
  if (isZeroValue(CurrNode) && isZeroValue(SuccNode)) {
    return getEdgeIdentity(Curr);
  }

  if (isZeroValue(CurrNode) && !isZeroValue(SuccNode)) {
    return getGenEdgeFunction(BBO);
  }

  if (EntryPoints.count(Curr->getFunction()->getName().str()) &&
      Curr == &Curr->getFunction()->front().front()) {
    // std::cout << "edge seed: " << CurrNode << " --> " << SuccNode
    //           << " with null\n";
    return getGenEdgeFunction(BBO);
  }

  auto [PointerOp, ValueOp] =
      [&]() -> std::tuple<const llvm::Value *, const llvm::Value *> {
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
      return {Store->getPointerOperand(), Store->getValueOperand()};
    }
    /// TODO: MemSetInst inherits from CallInst, so move it to summaryEF
    // if (const auto *MemSet = llvm::dyn_cast<llvm::MemSetInst>(Curr)) {
    //   return {MemSet->getDest(), MemSet->getValue()};
    // }
    return {nullptr, nullptr};
  }();

  if (PointerOp == nullptr) {
    return getEdgeIdentity(Curr);
  }

  assert(ValueOp);

  if (!disableStrongUpdates) {

    /// Kill the PointerOp, if we store into it
    if (CurrNode->mustAlias(makeFlowFact(PointerOp), *PT)) {
      return makeEF<GenEdgeFunction>(BBO, Curr);
    }

    auto SaniConfig = getSanitizerConfigAt(Curr);
    if (!SaniConfig.empty()) {
      // std::cerr << "NormalEF: handleEdgeConfig at " << llvmIRToString(Curr)
      //           << " on " << CurrNode << " --> " << SuccNode << "\n";
      if (isMustAlias(SaniConfig, CurrNode)) {
        return makeEF<GenEdgeFunction>(BBO, Curr);
      }
    }
  }

  // std::cerr << "StoreInst with EdgeID at " << llvmIRToString(Curr) << " on "
  //           << CurrNode << " --> " << SuccNode << "\n";

  return getEdgeIdentity(Curr);
}

auto IDEExtendedTaintAnalysis::getCallEdgeFunction(n_t CallInst, d_t SrcNode,
                                                   f_t CalleeFun, d_t DestNode)
    -> EdgeFunctionPtrType {

  if (disableStrongUpdates) {
    return getEdgeIdentity(CallInst);
  }
  if (isZeroValue(SrcNode) && isZeroValue(DestNode)) {
    return getEdgeIdentity(CallInst);
  }

  for (const auto &Arg : llvm::cast<llvm::CallBase>(CallInst)->args()) {
    // Kill sanitized facts that flow into the callee.
    if (equivalent(makeFlowFact(Arg.get()), SrcNode)) {
      return makeKillIfSanitizedEdgeFunction(BBO, getApproxLoadFrom(Arg.get()));
    }
  }

  return getEdgeIdentity(CallInst);
}

auto IDEExtendedTaintAnalysis::getReturnEdgeFunction(n_t CallSite,
                                                     f_t CalleeFun,
                                                     n_t ExitInst, d_t ExitNode,
                                                     n_t RetSite, d_t RetNode)
    -> EdgeFunctionPtrType {

  if (disableStrongUpdates) {
    return getEdgeIdentity(CallSite);
  }

  if (isZeroValue(ExitNode) && isZeroValue(RetNode)) {
    return getEdgeIdentity(CallSite);
  }

  if (const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(ExitInst);
      Ret && equivalent(RetNode, makeFlowFact(CallSite))) {
    return makeEF<TransferEdgeFunction>(
        BBO, getApproxLoadFrom(Ret->getReturnValue()), CallSite);
  }
  // Pointer parameters that have a sanitizer on all paths are always sanitized
  // at the return-site, no matter where the sanitizer is
  // return EdgeFunctionPtrType(new KillIfSanitizedEdgeFunction(BBO, nullptr));
  return makeEF<TransferEdgeFunction>(BBO, nullptr, CallSite);
}

auto IDEExtendedTaintAnalysis::getCallToRetEdgeFunction(
    n_t CallSite, d_t CallNode, n_t RetSite, d_t RetSiteNode,
    std::set<f_t> Callees) -> EdgeFunctionPtrType {

  // Intrinsics behave as they won't be there...
  bool isIntrinsic = std::all_of(Callees.begin(), Callees.end(),
                                 [](f_t Fn) { return Fn->isIntrinsic(); });

  if (!disableStrongUpdates && !isIntrinsic && CallNode == RetSiteNode) {
    // There was an Identity-Flow
    for (const auto &Arg : llvm::cast<llvm::CallBase>(CallSite)->args()) {
      if (Arg.get()->getType()->isPointerTy() &&
          equivalent(CallNode, makeFlowFact(Arg.get()))) {
        // kill the flow fact unconditionally
        return makeEF<GenEdgeFunction>(BBO, CallSite);
      }
    }
  }
  return getEdgeIdentity(CallSite);
}

auto IDEExtendedTaintAnalysis::getSummaryEdgeFunction(n_t Curr, d_t CurrNode,
                                                      n_t Succ, d_t SuccNode)
    -> EdgeFunctionPtrType {

  const auto *call = llvm::cast<llvm::CallBase>(Curr);

  if (isZeroValue(CurrNode) && !isZeroValue(SuccNode)) {
    return getGenEdgeFunction(BBO);
  }

  if (disableStrongUpdates) {
    return getEdgeIdentity(Curr);
  }

  llvm::SmallSet<const llvm::Function *, 2> callees;
  if (const auto *fn = call->getCalledFunction()) {
    callees.insert(fn);
  } else {
    base_t::ICF->forEachCalleeOfCallAt(
        Curr, [&callees](const llvm::Function *F) { callees.insert(F); });
  }

  SanitizerConfigTy SaniConfig;

  bool frst = true;
  for (const auto *F : callees) {
    auto tmp = getSanitizerConfigAt(Curr, F);
    if (frst) {
      frst = false;
      SaniConfig = std::move(tmp);
      continue;
    }

    intersectWith(SaniConfig, tmp);

    if (SaniConfig.empty()) {
      break;
    }
  }

  EdgeFunctionPtrType ret = getEdgeIdentity(Curr);

  if (isMustAlias(SaniConfig, CurrNode)) {
    return makeEF<GenEdgeFunction>(BBO, Curr);
  }

  /// TODO: MemSet

  return ret;
}

// Printing functions:

void IDEExtendedTaintAnalysis::printNode(std::ostream &os, n_t n) const {
  os << llvmIRToString(n);
}

void IDEExtendedTaintAnalysis::printDataFlowFact(std::ostream &os,
                                                 d_t d) const {
  os << d;
}

void IDEExtendedTaintAnalysis::printEdgeFact(std::ostream &os, l_t l) const {
  os << l;
}

void IDEExtendedTaintAnalysis::printFunction(std::ostream &os, f_t m) const {
  os << m->getName().str();
}

void IDEExtendedTaintAnalysis::emitTextReport(
    const SolverResults<n_t, d_t, l_t> &SR, std::ostream &OS) {
  OS << "===== IDEExtendedTaintAnalysis-Results =====\n";

  if (!postProcessed) {
    doPostProcessing(SR);
  }

  for (auto &[Inst, LeakSet] : Leaks) {
    OS << "At ";
    printNode(OS, Inst);
    OS << "\n";
    for (const auto &Leak : LeakSet) {
      OS << "\t" << llvmIRToShortString(Leak) << "\n";
    }
  }
  OS << std::endl;
}

// JoinLattice

auto IDEExtendedTaintAnalysis::topElement() -> l_t { return Top{}; }

auto IDEExtendedTaintAnalysis::bottomElement() -> l_t { return Bottom{}; }

auto IDEExtendedTaintAnalysis::join(l_t lhs, l_t rhs) -> l_t {
  return lhs.join(rhs, &BBO);
}

// Helpers:

auto IDEExtendedTaintAnalysis::makeFlowFact(const llvm::Value *V) -> d_t {
  return FactFactory.Create(V, bound);
}

void IDEExtendedTaintAnalysis::identity(std::set<d_t> &ret, const d_t &source,
                                        const llvm::Instruction *CurrInst,
                                        bool addGlobals) {

  if (addGlobals && llvm::isa<llvm::GlobalValue>(source->base())) {
    ret.insert(source);
  } else if (const auto *Inst =
                 llvm::dyn_cast<llvm::Instruction>(source->base());
             Inst && Inst->getFunction() == CurrInst->getFunction()) {
    ret.insert(source);
  } else if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(source->base());
             Arg && Arg->getParent() == CurrInst->getFunction()) {
    ret.insert(source);
  }
}

auto IDEExtendedTaintAnalysis::identity(const d_t &source,
                                        const llvm::Instruction *CurrInst,
                                        bool addGlobals) -> std::set<d_t> {
  std::set<d_t> ret;
  identity(ret, source, CurrInst, addGlobals);
  return ret;
}

auto IDEExtendedTaintAnalysis::transferFlowFact(d_t source, d_t From,
                                                const llvm::Value *To) -> d_t {

  return FactFactory.withTransferTo(source, From, To);
}

const llvm::Instruction *
IDEExtendedTaintAnalysis::getApproxLoadFrom(const llvm::Instruction *V) const {

  if (llvm::isa<llvm::LoadInst>(V) || llvm::isa<llvm::CallBase>(V)) {
    return V;
  }

  if (const auto *it = V->op_begin();
      it != V->op_end() && llvm::isa<llvm::Instruction>(*it)) {
    return getApproxLoadFrom(llvm::cast<llvm::Instruction>(*it));
  }

  return V;
}

const llvm::Instruction *
IDEExtendedTaintAnalysis::getApproxLoadFrom(const llvm::Value *V) const {
  if (V->getType()->isPointerTy()) {
    return nullptr;
  }

  if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
    return getApproxLoadFrom(Inst);
  }

  return nullptr;
}

void IDEExtendedTaintAnalysis::doPostProcessing(
    const SolverResults<n_t, d_t, l_t> &SR) {
  postProcessed = true;
  llvm::SmallVector<const llvm::Instruction *, 4> remInst;
  for (auto &[Inst, PotentialLeaks] : Leaks) {
    llvm::SmallVector<const llvm::Value *, 2> rem;
    std::cerr << "At " << llvmIRToString(Inst) << ":" << std::endl;

    auto results = SR.resultsAt(Inst);

    for (const auto *L : PotentialLeaks) {
      auto found = results.find(makeFlowFact(L));
      if (found == results.end()) {
        // The sanitizer has been killed, so we must assume the fact as tainted
        std::cerr << "No results for " << makeFlowFact(L) << std::endl;
        continue;
      }

      auto sani = // SR.resultAt(Inst, makeFlowFact(L));
          found->second;
      const auto *Load = getApproxLoadFrom(L);

      switch (sani.getKind()) {
      // case EdgeDomain::Bot:
      //   rem.push_back(L);
      //   std::cerr << "Sanitize " << llvmIRToShortString(L) << " with Bottom "
      //             << std::endl;
      //   break;
      case EdgeDomain::Sanitized:
        rem.push_back(L);
        std::cerr << "Sanitize " << llvmIRToShortString(L) << " from parent "
                  << std::endl;
        break;
      case EdgeDomain::WithSanitizer:
        if (!sani.getSanitizer()) {
          break;
        }
        if (!Load || BBO.mustComeBefore(sani.getSanitizer(), Load)) {
          rem.push_back(L);
          std::cerr << "Sanitize " << llvmIRToShortString(L) << " with "
                    << llvmIRToString(sani.getSanitizer()) << std::endl;
          break;
        }
        [[fallthrough]];
      default:
        std::cerr << " Sani: " << sani
                  << "; Load: " << (Load ? llvmIRToString(Load) : "null")
                  << " for FlowFact: " << makeFlowFact(L) << std::endl;
      }
    }
    std::cerr << "----------------------------" << std::endl;

    for (const auto *R : rem) {
      PotentialLeaks.erase(R);
    }
    if (PotentialLeaks.empty()) {
      remInst.push_back(Inst);
    }
  }
  for (const auto *Inst : remInst) {
    Leaks.erase(Inst);
  }
}

const LeakMap_t &IDEExtendedTaintAnalysis::getAllLeaks(
    IDESolver<IDEExtendedTaintAnalysisDomain> &Solver) & {
  if (!postProcessed) {
    doPostProcessing(Solver.getSolverResults());
  }
  return Leaks;
}

LeakMap_t IDEExtendedTaintAnalysis::getAllLeaks(
    IDESolver<IDEExtendedTaintAnalysisDomain> &Solver) && {
  if (!postProcessed) {
    doPostProcessing(Solver.getSolverResults());
  }
  return std::move(Leaks);
}

} // namespace psr::XTaint
