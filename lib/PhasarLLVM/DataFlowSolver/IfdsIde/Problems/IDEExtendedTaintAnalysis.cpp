/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <algorithm>
#include <type_traits>

#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Casting.h"

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

IDEExtendedTaintAnalysis::IDEExtendedTaintAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT, const TaintConfig *TSF,
    std::set<std::string> EntryPoints, unsigned Bound,
    bool DisableStrongUpdates)
    : base_t(IRDB, TH, ICF, PT, std::move(EntryPoints)), AnalysisBase(TSF),
      FactFactory(IRDB->getNumInstructions()),
      DL((*IRDB->getAllModules().begin())->getDataLayout()), Bound(Bound),
      PostProcessed(DisableStrongUpdates),
      DisableStrongUpdates(DisableStrongUpdates) {
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

bool IDEExtendedTaintAnalysis::isZeroValue(d_t Fact) const {
  return Fact->isZero();
}

IDEExtendedTaintAnalysis::EdgeFunctionPtrType
IDEExtendedTaintAnalysis::allTopFunction() {
  return getAllTop();
}

// Flow functions:

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getNormalFlowFunction(n_t Curr,
                                                [[maybe_unused]] n_t Succ) {

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "##Normal-FF at: " << psr::llvmIRToString(Curr));

  // The only instruction we need to handle in the Normal-FF is the StoreInst.
  // All other instructions are handled by the recursive Create function from
  // the FactFactory.

  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    auto StoreFF =
        getStoreFF(Store->getPointerOperand(), Store->getValueOperand(), Store);

    return StoreFF;
  }

  auto [SrcConfig, SinkConfig] = getConfigurationAt(Curr);
  if (!SrcConfig.empty() || !SinkConfig.empty()) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "handle config in normal-ff");
    return handleConfig(Curr, std::move(SrcConfig), std::move(SinkConfig));
  }

  return Identity<d_t>::getInstance();
}

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getStoreFF(const llvm::Value *PointerOp,
                                     const llvm::Value *ValueOp,
                                     const llvm::Instruction *Store,
                                     unsigned PALevel) {

  auto TV = makeFlowFact(ValueOp);
  auto PTS = this->PT->getPointsToSet(PointerOp, Store);

  auto Mem = makeFlowFact(PointerOp);
  return makeLambdaFlow<d_t>([this, TV, Mem, PTS{std::move(PTS)}, PointerOp,
                              ValueOp, Store,
                              PALevel](d_t Source) mutable -> std::set<d_t> {
    if (Source->isZero()) {
      std::set<d_t> Ret = {Source};
      generateFromZero(Ret, Store, PointerOp, ValueOp,
                       /*IncludeActualArg*/ false);
      return Ret;
    }
    /// Pointer-Arithetics in the last indirection are irrelevant for equality
    /// comparison:
    /// For example: Let source be a pointer to a tainted array-element and TV
    /// be a pointer to a different element of the same array. Then the taint is
    /// easily reachable from TV by simply doing dome pointer arithmetics.
    /// Hence, when loading the value of TV back from Mem this still holds and
    /// must be preserved by the analysis.
    if (TV->equivalentExceptPointerArithmetics(Source, PALevel)) {
      auto Offset = Source - TV;

      // generate all may-aliases of Store->getPointerOperand()
      std::set<d_t> Ret = {Source, FactFactory.withIndirectionOf(Mem, Offset)};

      for (const auto *Alias : *PTS) {
        if (llvm::isa<llvm::Constant>(Alias) || Alias == Mem->base()) {
          continue;
        }

        identity(Ret,
                 FactFactory.withIndirectionOf(makeFlowFact(Alias), Offset),
                 Store);
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Store generate: " << PrettyPrinter{Ret});

      // For the sink-variables, the pointer-arithmetics in the last offset
      // are relevant (in contrast to the Store-FF). This is, where the
      // field-sensitive results come from.
      if (TV->equivalent(Source)) {
        reportLeakIfNecessary(Store, PointerOp, ValueOp);
        for (const auto *Alias : *PTS) {
          reportLeakIfNecessary(Store, Alias, ValueOp);
        }
      }

#ifdef XTAINT_DIAGNOSTICS
      allTaintedValues.insert(ret.begin(), ret.end());
#endif

      return Ret;
    }
    // Sanitizing is handled in the edge function

    return {Source};
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
    std::set<d_t> Ret = {Source};

    if (Source->isZero()) {
      for (const auto *Src : SourceConfig) {
        Ret.insert(makeFlowFact(Src));
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

    return Ret;
  });
}

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getCallFlowFunction(n_t CallStmt, f_t DestFun) {
  const auto *call = llvm::cast<llvm::CallBase>(CallStmt);
  assert(call);
  if (DestFun->isDeclaration()) {
    return Identity<d_t>::getInstance();
  }
  bool HasVarargs = call->arg_size() > DestFun->arg_size();
  const auto *const VA = [&]() -> const llvm::Value * {
    if (!HasVarargs) {
      return nullptr;
    }
    // Copied from IDELinearConstantAnalysis:
    // Over-approximate by trying to add the
    //   alloca [1 x %struct.__va_list_tag], align 16
    // to the results
    // find the allocated %struct.__va_list_tag and generate it
    for (auto It = llvm::inst_begin(DestFun), End = llvm::inst_end(DestFun);
         It != End; ++It) {
      if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(&*It)) {
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

  return makeLambdaFlow<d_t>([this, call, DestFun,
                              VA](d_t Source) -> std::set<d_t> {
    if (isZeroValue(Source)) {
      return {Source};
    }
    std::set<d_t> Ret;

    using ArgIterator = llvm::User::const_op_iterator;
    using ParamIterator = llvm::Function::const_arg_iterator;

    ArgIterator It = call->arg_begin();
    ArgIterator End = call->arg_end();
    ParamIterator FIt = DestFun->arg_begin();
    ParamIterator FEnd = DestFun->arg_end();

    const std::string CalleeName =
        (DestFun->hasName()) ? DestFun->getName().str() : "none";
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "##Call-FF at: " << psr::llvmIRToString(call)
                  << " to: " << CalleeName);
    for (; FIt != FEnd && It != End; ++FIt, ++It) {
      auto From = makeFlowFact(It->get());
      /// Pointer-Arithetics in the last indirection are irrelevant for
      /// equality comparison. Argumentation similar to StoreFF
      if (equivalentExceptPointerArithmetics(From, Source)) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << ">\tmatch: " << From << " vs " << Source);
        Ret.insert(transferFlowFact(Source, From, &*FIt));
      } else {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << ">\tno match: " << From << " vs " << Source);
      }
    }
    ptrdiff_t Offs = 0;

    if (!VA) {
      return Ret;
    }

    for (; It != End; ++It) {
      auto From = makeFlowFact(It->get());
      if (equivalentExceptPointerArithmetics(From, Source)) {
        auto To = transferFlowFact(Source, From, VA);

        Ret.insert(FactFactory.withIndirectionOf(To, {Offs}));
        /// Model varargs as an additional aggregate-parameter. The varargs
        /// are stored in contiguous memory one after the other. Ignore
        /// padding for now.
      }
      Offs +=
          ptrdiff_t(DL.getTypeAllocSize(It->get()->getType()).getFixedSize());
    }
#ifdef XTAINT_DIAGNOSTICS
    allTaintedValues.insert(ret.begin(), ret.end());
#endif
    return Ret;
  });
}

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                             n_t ExitStmt,
                                             [[maybe_unused]] n_t RetSite) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "##Return-FF at: " << psr::llvmIRToString(CallSite));
  const auto *Call = llvm::cast<llvm::CallBase>(CallSite);
  return makeLambdaFlow<d_t>([this, Call, CalleeFun,
                              ExitStmt{llvm::cast<llvm::ReturnInst>(ExitStmt)}](
                                 d_t Source) {
    std::set<d_t> Ret;

    using ArgIterator = llvm::User::const_op_iterator;
    using ParamIterator = llvm::Function::const_arg_iterator;

    ArgIterator It = Call->arg_begin();
    ArgIterator End = Call->arg_end();
    ParamIterator FIt = CalleeFun->arg_begin();
    ParamIterator FEnd = CalleeFun->arg_end();

    for (; FIt != FEnd && It != End; ++FIt, ++It) {
      // Only map back pointer parameters, since for all others we have
      // call-by-value.
      if (FIt->getType()->isPointerTy() &&
          equivalent(Source, makeFlowFact(&*FIt))) {
        Ret.insert(
            FactFactory.withTransferFrom(Source, makeFlowFact(It->get())));
      }
    }
    // For now, ignore mapping back varargs
    d_t RetVal;
    if (ExitStmt->getReturnValue() &&
        equivalent(Source, RetVal = makeFlowFact(ExitStmt->getReturnValue()))) {
      Ret.insert(FactFactory.withOffsets(makeFlowFact(Call), Source - RetVal));
    }

#ifdef XTAINT_DIAGNOSTICS
    allTaintedValues.insert(ret.begin(), ret.end());
#endif

    return Ret;
  });
}

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getCallToRetFlowFunction(
    [[maybe_unused]] n_t CallSite, [[maybe_unused]] n_t RetSite,
    [[maybe_unused]] std::set<f_t> Callees) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "##CallToReturn-FF at: " << psr::llvmIRToString(CallSite));
  // The CTR-FF is traditionally an identity function. All CTR-relevant stuff is
  // handled on the edges.

  return Identity<d_t>::getInstance();
}

IDEExtendedTaintAnalysis::FlowFunctionPtrType
IDEExtendedTaintAnalysis::getSummaryFlowFunction(n_t CallStmt, f_t DestFun) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "##Summary-FF at: " << psr::llvmIRToString(CallStmt));
  // Handle all the functions that have a special semantics inside the analysis:
  // - Calls to the DevAPI
  // - Calls to sources, sinks and sanitizers
  // If the seeds were autogenerated, we can ignore calls to the DevAPI here,
  // since they were already considered when constructing the seeds

  auto [SrcConfig, SinkConfig] = getConfigurationAt(CallStmt, DestFun);
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "SrcIndices.any(): " << !SrcConfig.empty()
                << " - SinkIndices.any(): " << !SinkConfig.empty());
  if (!SrcConfig.empty() || !SinkConfig.empty()) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "handle config in summary-ff");
    return handleConfig(CallStmt, std::move(SrcConfig), std::move(SinkConfig));
  }

  if (const auto *MemSet = llvm::dyn_cast<llvm::MemSetInst>(CallStmt)) {
    /// Basically, MemSet is the same as Store
    return getStoreFF(MemSet->getRawDest(), MemSet->getValue(), MemSet);
  }

  if (const auto *MemTrn = llvm::dyn_cast<llvm::MemTransferInst>(CallStmt)) {
    /// Basically, MemCpy/MemMove are the same as Store.
    /// We just need to take care about the additional level of indirection
    /// i.e., not the source itself is stored, but the value it is pointing to
    return getStoreFF(MemTrn->getRawDest(), MemTrn->getRawSource(), MemTrn,
                      /*PALevel*/ 2);
  }

  return nullptr;
}

// Edge Functions:

auto IDEExtendedTaintAnalysis::getNormalEdgeFunction(n_t Curr, d_t CurrNode,
                                                     [[maybe_unused]] n_t Succ,
                                                     d_t SuccNode)
    -> EdgeFunctionPtrType {
  if (isZeroValue(CurrNode) && isZeroValue(SuccNode)) {
    return getEdgeIdentity(Curr);
  }

  if (isZeroValue(CurrNode) && !isZeroValue(SuccNode)) {
    return getGenEdgeFunction(BBO);
  }

  if (EntryPoints.count(Curr->getFunction()->getName().str()) &&
      Curr == &Curr->getFunction()->front().front()) {
    return getGenEdgeFunction(BBO);
  }

  auto [PointerOp, ValueOp] =
      [&]() -> std::tuple<const llvm::Value *, const llvm::Value *> {
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
      return {Store->getPointerOperand(), Store->getValueOperand()};
    }

    return {nullptr, nullptr};
  }();

  if (PointerOp == nullptr) {
    return getEdgeIdentity(Curr);
  }

  assert(ValueOp);

  if (!DisableStrongUpdates) {

    /// Kill the PointerOp, if we store into it
    if (CurrNode->mustAlias(makeFlowFact(PointerOp), *PT)) {
      return makeEF<GenEdgeFunction>(BBO, Curr);
    }

    auto SaniConfig = getSanitizerConfigAt(Curr);
    if (!SaniConfig.empty()) {
      if (isMustAlias(SaniConfig, CurrNode)) {
        return makeEF<GenEdgeFunction>(BBO, Curr);
      }
    }
  }

  return getEdgeIdentity(Curr);
}

auto IDEExtendedTaintAnalysis::getCallEdgeFunction(
    n_t CallInst, d_t SrcNode, [[maybe_unused]] f_t CalleeFun, d_t DestNode)
    -> EdgeFunctionPtrType {

  if (DisableStrongUpdates) {
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

auto IDEExtendedTaintAnalysis::getReturnEdgeFunction(
    n_t CallSite, [[maybe_unused]] f_t CalleeFun, n_t ExitInst, d_t ExitNode,
    [[maybe_unused]] n_t RetSite, d_t RetNode) -> EdgeFunctionPtrType {

  if (DisableStrongUpdates) {
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
    n_t CallSite, d_t CallNode, [[maybe_unused]] n_t RetSite, d_t RetSiteNode,
    std::set<f_t> Callees) -> EdgeFunctionPtrType {

  // Intrinsics behave as they won't be there...
  bool IsIntrinsic = std::all_of(Callees.begin(), Callees.end(),
                                 [](f_t Fn) { return Fn->isIntrinsic(); });

  if (!DisableStrongUpdates && !IsIntrinsic && CallNode == RetSiteNode) {
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
                                                      [[maybe_unused]] n_t Succ,
                                                      d_t SuccNode)
    -> EdgeFunctionPtrType {

  const auto *Call = llvm::cast<llvm::CallBase>(Curr);

  if (isZeroValue(CurrNode) && !isZeroValue(SuccNode)) {
    return getGenEdgeFunction(BBO);
  }

  if (DisableStrongUpdates) {
    return getEdgeIdentity(Curr);
  }

  llvm::SmallSet<const llvm::Function *, 2> Callees;
  if (const auto *Fn = Call->getCalledFunction()) {
    Callees.insert(Fn);
  } else {
    base_t::ICF->forEachCalleeOfCallAt(
        Curr, [&Callees](const llvm::Function *F) { Callees.insert(F); });
  }

  SanitizerConfigTy SaniConfig;

  bool Frst = true;
  for (const auto *F : Callees) {
    auto Tmp = getSanitizerConfigAt(Curr, F);
    if (Frst) {
      Frst = false;
      SaniConfig = std::move(Tmp);
      continue;
    }

    intersectWith(SaniConfig, Tmp);

    if (SaniConfig.empty()) {
      break;
    }
  }

  EdgeFunctionPtrType Ret = getEdgeIdentity(Curr);

  if (isMustAlias(SaniConfig, CurrNode)) {
    return makeEF<GenEdgeFunction>(BBO, Curr);
  }

  // MemIntrinsic covers memset, memcpy and memmove
  if (const auto *MemSet = llvm::dyn_cast<llvm::MemIntrinsic>(Curr);
      MemSet && CurrNode->mustAlias(makeFlowFact(MemSet->getRawDest()), *PT)) {
    return makeEF<GenEdgeFunction>(BBO, Curr);
  }

  return Ret;
}

// Printing functions:

void IDEExtendedTaintAnalysis::printNode(std::ostream &OS, n_t Inst) const {
  OS << llvmIRToString(Inst);
}

void IDEExtendedTaintAnalysis::printDataFlowFact(std::ostream &OS,
                                                 d_t Fact) const {
  OS << Fact;
}

void IDEExtendedTaintAnalysis::printEdgeFact(std::ostream &OS, l_t Fact) const {
  OS << Fact;
}

void IDEExtendedTaintAnalysis::printFunction(std::ostream &OS, f_t Fun) const {
  OS << Fun->getName().str();
}

void IDEExtendedTaintAnalysis::emitTextReport(
    const SolverResults<n_t, d_t, l_t> &SR, std::ostream &OS) {
  OS << "===== IDEExtendedTaintAnalysis-Results =====\n";

  if (!PostProcessed) {
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

auto IDEExtendedTaintAnalysis::join(l_t LHS, l_t RHS) -> l_t {
  return LHS.join(RHS, &BBO);
}

// Helpers:

auto IDEExtendedTaintAnalysis::makeFlowFact(const llvm::Value *V) -> d_t {
  return FactFactory.Create(V, Bound);
}

void IDEExtendedTaintAnalysis::identity(std::set<d_t> &Ret, const d_t &Source,
                                        const llvm::Instruction *CurrInst,
                                        bool AddGlobals) {

  if (AddGlobals && llvm::isa<llvm::GlobalValue>(Source->base())) {
    Ret.insert(Source);
  } else if (const auto *Inst =
                 llvm::dyn_cast<llvm::Instruction>(Source->base());
             Inst && Inst->getFunction() == CurrInst->getFunction()) {
    Ret.insert(Source);
  } else if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(Source->base());
             Arg && Arg->getParent() == CurrInst->getFunction()) {
    Ret.insert(Source);
  }
}

auto IDEExtendedTaintAnalysis::identity(const d_t &Source,
                                        const llvm::Instruction *CurrInst,
                                        bool AddGlobals) -> std::set<d_t> {
  std::set<d_t> Ret;
  identity(Ret, Source, CurrInst, AddGlobals);
  return Ret;
}

auto IDEExtendedTaintAnalysis::transferFlowFact(d_t Source, d_t From,
                                                const llvm::Value *To) -> d_t {

  return FactFactory.withTransferTo(Source, From, To);
}

const llvm::Instruction *
IDEExtendedTaintAnalysis::getApproxLoadFrom(const llvm::Instruction *V) const {

  if (llvm::isa<llvm::LoadInst>(V) || llvm::isa<llvm::CallBase>(V)) {
    return V;
  }

  if (const auto *It = V->op_begin();
      It != V->op_end() && llvm::isa<llvm::Instruction>(*It)) {
    return getApproxLoadFrom(llvm::cast<llvm::Instruction>(*It));
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
  PostProcessed = true;
  llvm::SmallVector<const llvm::Instruction *> RemInst;
  for (auto &[Inst, PotentialLeaks] : Leaks) {
    llvm::SmallVector<const llvm::Value *, 2> Rem;
    std::cerr << "At " << llvmIRToString(Inst) << ":" << std::endl;

    auto Results = SR.resultsAt(Inst);

    for (const auto *L : PotentialLeaks) {
      auto Found = Results.find(makeFlowFact(L));
      if (Found == Results.end()) {
        // The sanitizer has been killed, so we must assume the fact as tainted
        std::cerr << "No results for " << makeFlowFact(L) << std::endl;
        continue;
      }

      auto Sani = // SR.resultAt(Inst, makeFlowFact(L));
          Found->second;
      const auto *Load = getApproxLoadFrom(L);

      switch (Sani.getKind()) {
      case EdgeDomain::Sanitized:
        Rem.push_back(L);
        std::cerr << "Sanitize " << llvmIRToShortString(L) << " from parent "
                  << std::endl;
        break;
      case EdgeDomain::WithSanitizer:
        if (!Sani.getSanitizer()) {
          break;
        }
        if (!Load || BBO.mustComeBefore(Sani.getSanitizer(), Load)) {
          Rem.push_back(L);
          std::cerr << "Sanitize " << llvmIRToShortString(L) << " with "
                    << llvmIRToString(Sani.getSanitizer()) << std::endl;
          break;
        }
        [[fallthrough]];
      default:
        std::cerr << " Sani: " << Sani
                  << "; Load: " << (Load ? llvmIRToString(Load) : "null")
                  << " for FlowFact: " << makeFlowFact(L) << std::endl;
      }
    }
    std::cerr << "----------------------------" << std::endl;

    for (const auto *R : Rem) {
      PotentialLeaks.erase(R);
    }
    if (PotentialLeaks.empty()) {
      RemInst.push_back(Inst);
    }
  }
  for (const auto *Inst : RemInst) {
    Leaks.erase(Inst);
  }
}

const LeakMap_t &IDEExtendedTaintAnalysis::getAllLeaks(
    IDESolver<IDEExtendedTaintAnalysisDomain> &Solver) & {
  if (!PostProcessed) {
    doPostProcessing(Solver.getSolverResults());
  }
  return Leaks;
}

LeakMap_t IDEExtendedTaintAnalysis::getAllLeaks(
    IDESolver<IDEExtendedTaintAnalysisDomain> &Solver) && {
  if (!PostProcessed) {
    doPostProcessing(Solver.getSolverResults());
  }
  return std::move(Leaks);
}

} // namespace psr::XTaint
