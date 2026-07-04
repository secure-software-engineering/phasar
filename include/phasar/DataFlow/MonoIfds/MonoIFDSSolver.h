#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/CFG.h"
#include "phasar/ControlFlow/CGSCCs.h"
#include "phasar/ControlFlow/ControlFlowOrder.h"
#include "phasar/ControlFlow/ICFG.h"
#include "phasar/DataFlow/HelperAnalyses.h"
#include "phasar/DataFlow/MonoIfds/ArraySetWorkList.h"
#include "phasar/DataFlow/MonoIfds/DataFlowEnvironment.h"
#include "phasar/DataFlow/MonoIfds/MonoIFDSConfig.h"
#include "phasar/DataFlow/MonoIfds/MonoIFDSProblem.h"
#include "phasar/DataFlow/MonoIfds/RPOWorkList.h"
#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/FunctionId.h"
#include "phasar/Utils/HashUtils.h"
#include "phasar/Utils/Lazy.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/Nullable.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/RepeatIterator.h"
#include "phasar/Utils/SCCGeneric.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/TypeName.h"

#include <memory>
#include <memory_resource>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace psr::monoifds {

class MonoIFDFSSolverBase {
public:
  static constexpr llvm::StringLiteral LogCategory = "MonoIFDSSolver";
};

/// \brief Implements the MonoIFDS algorithm, as presented in "Scaling Bottom-up
/// IFDS Taint Analysis with Optimized Data-flow Encoding" by Schiebel and
/// Bodden. <TODO: DOI>
template <MonoIFDSProblem ProblemT,
          ICFG ICFGTy = typename ProblemT::ProblemAnalysisDomain::i_t>
class MonoIFDSSolver : public MonoIFDFSSolverBase {
public:
  using n_t = typename ProblemT::ProblemAnalysisDomain::n_t;
  using d_t = typename ProblemT::ProblemAnalysisDomain::d_t;
  using i_t = ICFGTy;
  using f_t = typename ProblemT::ProblemAnalysisDomain::f_t;

  explicit MonoIFDSSolver(ProblemT *Problem, const i_t *ICF,
                          std::pmr::polymorphic_allocator<> Alloc =
                              std::pmr::get_default_resource())
      : Problem(&assertNotNull(Problem)), ICF(&assertNotNull(ICF)),
        MBufRes(Alloc.resource()) {}

  template <CanGetICFGOf<n_t, f_t> HelperAnalysesT>
  explicit MonoIFDSSolver(ProblemT *Problem, HelperAnalysesT &HA,
                          std::pmr::polymorphic_allocator<> Alloc =
                              std::pmr::get_default_resource())
      : MonoIFDSSolver(Problem, &HA.getICFG(), Alloc) {
    if constexpr (CanGetCompressedFunctionsOf<HelperAnalysesT, f_t>) {
      setFunctionCompressor(&HA.getCompressedFunctions());
    }
    if constexpr (CanGetCGSCCs<HelperAnalysesT>) {
      setCGSCCs(&HA.getCGSCCs());
    }
  }

  MonoIFDSSolver &setConfig(MonoIfdsConfig Config) & noexcept {
    this->Config = Config;
    return *this;
  }

  MonoIFDSSolver &setCGSCCs(const SCCHolder<FunctionId> *SCCs) & noexcept {
    assertNotNull(SCCs);
    this->SCCs = SCCs;
    return *this;
  }

  MonoIFDSSolver &setFunctionCompressor(
      const Compressor<f_t, FunctionId> *Functions) & noexcept {
    assertNotNull(Functions);
    this->Functions = Functions;
    return *this;
  }

  void solve();

  void dumpResults(llvm::raw_ostream &OS) const {
    OS << "No Raw-Results Dump available yet!\n";
  }

  void emitTextReport(llvm::raw_ostream &OS) const {
    Problem->emitTextReport(OS);
  }

private:
  // NOTE: Used the node_hash_map from
  // [parallel-hash-map](https://github.com/greg7mdp/parallel-hashmap) here
  // for the paper-eval!
  template <typename Key, typename Value>
  using node_hash_map =
      std::pmr::unordered_map<Key, Value, psr::DefaultHash<Key>>;

  struct FunctionSummary {
    Compressor<d_t, SourceFactId> SourceFactIds;
    DataFlowEnvironment<d_t> EndSummary;

    node_hash_map<std::pair<n_t, d_t>, SourceFactSet> LeakIf;

    FunctionSummary(std::pmr::memory_resource *MRes) : LeakIf(MRes) {}
  };

  using LocalAnalysis = decltype(std::declval<ProblemT>().localAnalysis(
      SCCId<FunctionId>(), std::declval<std::pmr::memory_resource *>()));

  struct IntermediateState {
    LocalAnalysis LocalProblem;
    node_hash_map<n_t, DataFlowEnvironment<d_t>> PathEdges;
    node_hash_map<f_t, llvm::SmallDenseSet<n_t>> Incoming;

    llvm::SmallDenseSet<FunctionId> HasNewLeaks;
    llvm::SmallDenseSet<FunctionId> HasNewSummary;

    SCCId<FunctionId> CurrSCC;
    bool InRecursion;

    IntermediateState(ProblemT *Problem, std::pmr::memory_resource *MRes,
                      SCCId<FunctionId> CurrSCC, bool InRecursion)
        : LocalProblem(Problem->localAnalysis(CurrSCC, MRes)), PathEdges(MRes),
          Incoming(MRes), CurrSCC(CurrSCC), InRecursion(InRecursion) {}
  };

  struct Mapper {
    TypedVector<SourceFactId, SourceFactSet> Mapping;
    BitSet<SourceFactId> ComputedMappings;

    explicit Mapper(size_t NumCalleeSrcFacts) {
      Mapping.resize(NumCalleeSrcFacts);
      ComputedMappings.reserve(NumCalleeSrcFacts);
    }

    void reset() {
      for (auto &SrcFacts : Mapping) {
        SrcFacts.clear();
      }
      ComputedMappings.clear();
    }

    const SourceFactSet &getSourceFactsFor(auto &LocalProblem,
                                           const DataFlowEnvironment<d_t> &In,
                                           const FunctionSummary &CalleeSum,
                                           SourceFactId CalleeSrc,
                                           ByConstRef<n_t> CallInst) {
      auto &Ret = Mapping[CalleeSrc];

      if (ComputedMappings.tryInsert(CalleeSrc)) {
        auto &&CSFacts = LocalProblem.invCallFlow(
            CallInst, CalleeSum.SourceFactIds[CalleeSrc]);

        for (const auto *Fact : CSFacts) {
          if (const auto *FactSrc = getOrNull(In, Fact)) {
            Ret.insertAllOf(*FactSrc);
          }
        }
      }

      return Ret;
    }

    void insertAllSrcFactsFor(SourceFactSet &Into, auto &LocalProblem,
                              const DataFlowEnvironment<d_t> &In,
                              const FunctionSummary &CalleeSum,
                              const SourceFactSet &CalleeSrcs,
                              ByConstRef<n_t> CallInst) {
      CalleeSrcs.foreach ([&](auto SrcFactId) {
        Into.insertAllOf(getSourceFactsFor(LocalProblem, In, CalleeSum,
                                           SrcFactId, CallInst));
      });
    }

    [[nodiscard]] SourceFactSet
    getAllSrcFactsFor(auto &LocalProblem, const DataFlowEnvironment<d_t> &In,
                      const FunctionSummary &CalleeSum,
                      const SourceFactSet &CalleeSrcs,
                      ByConstRef<n_t> CallInst) {
      SourceFactSet Ret;
      insertAllSrcFactsFor(Ret, LocalProblem, In, CalleeSum, CalleeSrcs,
                           CallInst);
      return Ret;
    }
  };

  void initializeFunctions() {
    if (SCCs) {
      throw std::logic_error("SCCs without FunctionCompressor?");
    }

    if constexpr (requires() {
                    {
                      Problem->getEntryPoints()
                    } -> psr::is_iterable_over_v<f_t>;
                  }) {
      Functions = std::make_unique<FunctionCompressor<f_t>>(
          compressFunctions(ICF->getCallGraph(), Problem->getEntryPoints()));
    } else if constexpr (requires() {
                           {
                             Problem->getEntryPoints()
                           } -> psr::is_iterable_over_v<std::string>;
                         }) {
      Functions = std::make_unique<FunctionCompressor<f_t>>(compressFunctions(
          ICF->getCallGraph(),
          psr::getEntryFunctions(*ICF, Problem->getEntryPoints())));
    } else {
      throw std::logic_error("The analysis problem " +
                             llvm::getTypeName<ProblemT>().str() +
                             " does not provide getEntryPoints(). So, you "
                             "must set a FunctionCompressor by calling "
                             "setFunctionCompressor() on the solver!");
    }
  }

  void initializeSCCs() {
    assert(Functions && "Functions have been initialized already (see "
                        "invocation order in initialize())");
    SCCs = std::make_unique<SCCHolder<FunctionId>>(
        computeCGSCCs(ICF->getCallGraph(), *ICF, *Functions));
  }

  void initialize() {
    if (!Functions) {
      initializeFunctions();
    }
    if (!SCCs) {
      initializeSCCs();
    }
  }

  void computeFixpointForSCC(SCCId<FunctionId> CurrSCC,
                             llvm::ArrayRef<FunctionId> CurrFuns) {

    const size_t SCCSize = CurrFuns.size();
    const bool InRecursion = SCCSize > 1;
    IntermediateState IState(Problem, &PoolRes, CurrSCC, InRecursion);

    const auto IterStrategy = Config.IterStrategy;
    const bool UseTopoFixpointDriver = [=] {
      if (IterStrategy == IterationStrategy::DedupFIFOQueue) {
        return false;
      }

      if (IterStrategy == IterationStrategy::Hybrid) {
        // return SCCSize < 20;
        return SCCSize == 1;
      }

      return true;
    }();

    ControlFlowOrder<n_t> CFO;
    if (UseTopoFixpointDriver) {
      for (const auto &Fun : CurrFuns) {
        computeCFGOrder(CFO, *ICF, (*Functions)[Fun]);
      }
    }

    ArraySetDriver<n_t> DefaultDriver;
    TopoFixpointDriver<n_t> TopoDriver;

    const auto ComputeFixpointWithDriver =
        [&](auto &Driver) LLVM_ATTRIBUTE_NOINLINE {
          for (auto FunId : llvm::reverse(CurrFuns)) {
            const auto *Fun = (*Functions)[FunId];
            submitInitialSeeds(IState, Driver, Summaries[FunId].SourceFactIds,
                               Fun);
          }
          Driver.run([&](n_t BlockStart) {
            analyzeBlock(IState, Driver, BlockStart);
          });
          assert(Driver.empty());

          // llvm::errs() << '.';
        };

    const auto RepropagateInRecursion = [&](auto &Driver) {
      rescheduleCalls(IState, Driver);
      while (!Driver.empty()) {
        Driver.run(
            [&](n_t BlockStart) { analyzeBlock(IState, Driver, BlockStart); });
        assert(Driver.empty());

        rescheduleCalls(IState, Driver);
        // llvm::errs() << '.';
      }

      assert(IState.HasNewSummary.empty() &&
             "After repropagating, we should not have any summary "
             "applications pending");
    };
    if (UseTopoFixpointDriver) {
      ComputeFixpointWithDriver(TopoDriver);
    } else {
      ComputeFixpointWithDriver(DefaultDriver);
    }

    if (!Config.EagerReturnPropagation) {
      if (IterStrategy == IterationStrategy::TopoPrioQueue) {
        RepropagateInRecursion(TopoDriver);
      } else {
        RepropagateInRecursion(DefaultDriver);
      }
    }

    repropagateLeaks(IState, CurrSCC);
  }

  /// Lines 1-3 in Algorithm 4
  void submitInitialSeeds(IntermediateState &IState, auto &Driver,
                          Compressor<d_t, SourceFactId> &SeedCompressor,
                          ByConstRef<f_t> Fun) {
    PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                         "[submitInitialSeeds]: For fun " << FToString(Fun));
    const auto &SPs = ICF->getStartPointsOf(Fun);

    const auto &Zero = IState.LocalProblem.getZeroValue();
    SeedCompressor.insert(Zero);
    assert(SeedCompressor.get(Zero) == SourceFactId(0) &&
           "The Zero value must always have Id 0!");

    for (const auto &SP : SPs) {
      auto &SeedState = IState.PathEdges[SP];
      SeedState[Zero].insert(SourceFactId(0));

      IState.LocalProblem.initialSeeds(SeedState, SeedCompressor, Fun);
      Driver.push(SP);
    }
  }

  /// Delayed Line 39 in Algorithm 4
  void rescheduleCalls(IntermediateState &IState, auto &Driver) {
    if (!IState.InRecursion) {
      return;
    }

    const bool EnableEnvVersioning = Config.EnableEnvVersioning;

    for (auto FunId : IState.HasNewSummary) {
      IState.HasNewLeaks.erase(FunId);
      const auto &Fun = (*Functions)[FunId];

      for (const auto &CS : getOrDefault(IState.Incoming, Fun)) {
        const auto &CSFun = ICF->getFunctionOf(CS);
        if (auto CallerId = Functions->getOrNull(CSFun)) {
          Driver.push(CS);
          if (EnableEnvVersioning) {
            IState.PathEdges[CS].Version++;
          }
        }
      }
    }
    IState.HasNewSummary.clear();
  }

  /// RepropagateLeaks procedure in Algorithm 2
  void repropagateLeaks(IntermediateState &IState, SCCId<FunctionId> CurrSCC) {
    llvm::SmallDenseSet<FunctionId> NewLeaksWL;
    while (!IState.HasNewLeaks.empty()) {
      NewLeaksWL.swap(IState.HasNewLeaks);

      for (auto FunId : NewLeaksWL) {
        handleLeaksForFun(IState, CurrSCC, FunId);
      }
      NewLeaksWL.clear();
    }
  }

  /// Continuation of RepropagateLeaks procedure in Algorithm 2
  void handleLeaksForFun(IntermediateState &IState, SCCId<FunctionId> CurrSCC,
                         FunctionId CurrFun) {
    const auto *Fun = (*Functions)[CurrFun];
    const auto &Sum = Summaries[CurrFun];

    Mapper M(Sum.SourceFactIds.size());

    for (const auto &CS : ICF->getCallersOf(Fun)) {
      auto CallerId = Functions->getOrNull(CS->getFunction());
      if (!CallerId) {
        continue;
      }

      auto CallerSCC = SCCs->SCCOfNode[*CallerId];
      if (CallerSCC != CurrSCC) {
        continue;
      }

      M.reset();

      const auto &In = getOrDefault(IState.PathEdges, CS);

      for (const auto &[CalleeLeak, LeakSrc] : Sum.LeakIf) {
        const auto &CSSrc =
            M.getAllSrcFactsFor(IState.LocalProblem, In, Sum, LeakSrc, CS);
        reportOrPropagateLeak(IState, *CallerId, CalleeLeak.first,
                              CalleeLeak.second, CSSrc);
      }
    }
  }

  void analyzeBlock(IntermediateState &IState, auto &Driver,
                    ByConstRef<n_t> BlockStart) {

    auto &LocalStateRef = IState.PathEdges[BlockStart];
    if (Config.EnableEnvVersioning &&
        LocalStateRef.AnalyzedVersion >= LocalStateRef.Version) {
      // Nothing to be done here
      return;
    }

    LocalStateRef.AnalyzedVersion = LocalStateRef.Version;
    analyzeBlockImpl(IState, Driver, BlockStart, LocalStateRef);
  }

  /// Procedure AnalyzeBlock (Lines 8-11+14 in Algorithm 4)
  void analyzeBlockImpl(IntermediateState &IState, auto &Driver,
                        ByConstRef<n_t> BlockStart,
                        DataFlowEnvironment<d_t> LocalState) {

    auto CurrFunId = Functions->get(BlockStart->getFunction());

    Nullable<n_t> CurrInst = BlockStart;

    do {
      auto Last = unwrapNullable(CurrInst);

      do {
        auto Curr = unwrapNullable(CurrInst);
        analyzeInstruction(IState, LocalState, CurrFunId, Curr);
        Last = Curr;
        if constexpr (IsBlockAwareControlFlow<i_t>) {
          CurrInst = ICF->getUniqueSuccessor(Curr);
        } else {
          const auto &Succs = ICF->getSuccsOf(Curr);
          if (Succs.size() == 1) {
            CurrInst = Succs[0];
          } else {
            CurrInst = {};
          }
        }
      } while (CurrInst);

      Nullable<n_t> UniqueSucc{};

      // We have at least one instruction, so we can safely unwrap here
      const auto &Succs = ICF->getSuccsOf(Last);
      const auto SuccSz = Succs.size();
      const bool HasSingleSucc = SuccSz == 1;
      for (const auto &Succ : Succs) {
        bool HasSinglePred = [&]() {
          if constexpr (IsBlockAwareControlFlow<i_t>) {
            return ICF->hasUniquePredecessor(Succ);
          }
          return false;
        }();

        auto [SuccBBStateIt, Inserted] = IState.PathEdges.try_emplace(
            Succ, lazy{[&] {
              if (HasSingleSucc && !(HasSinglePred && !UniqueSucc)) {
                return std::move(LocalState);
              }

              return LocalState;
            }});

        if (HasSinglePred) {
          // Assign

          if (Inserted || SuccBBStateIt->second != LocalState) {
            if (!UniqueSucc) {
              UniqueSucc = Succ;
              if (!Inserted) {
                // Note: Cannot move LocalState here, as we still
                // need it in the next iteration
                SuccBBStateIt->second = LocalState;
              }

            } else {
              Driver.push(Succ);
              if (!Inserted) {
                if (HasSingleSucc) {
                  SuccBBStateIt->second = std::move(LocalState);
                } else {
                  SuccBBStateIt->second = LocalState;
                }
              }
            }

            SuccBBStateIt->second.Version++;
          }
          continue;
        }

        // Merge
        if (Inserted || tryMergeStates(SuccBBStateIt->second, LocalState)) {
          SuccBBStateIt->second.Version++;
          Driver.push(Succ);
        }
      }

      if (SuccSz == 0 && Config.EagerReturnPropagation &&
          ICF->isExitInst(Last)) {
        if (IState.HasNewSummary.erase(CurrFunId)) {
          rescheduleCallsAtExit(IState, Driver, CurrFunId);
        }
      }

      CurrInst = UniqueSucc;
    } while (CurrInst);
  }

  /// Lines 15-20 in Algorithm 4
  void analyzeInstruction(IntermediateState &IState,
                          DataFlowEnvironment<d_t> &LocalState,
                          FunctionId CurrFunId, ByConstRef<n_t> Inst) {

    if (ICF->isCallSite(Inst)) {
      return analyzeCallInst(IState, LocalState, CurrFunId, Inst);
    }

    handleSourceSinkConfig(IState, LocalState, CurrFunId, Inst);

    if (ICF->isExitInst(Inst)) {
      return analyzeExitInst(IState, LocalState, CurrFunId, Inst);
    }

    IState.LocalProblem.normalFlow(LocalState, Inst);
  }

  /// Procedure AnalyzeExit (Lines 35-38 in Algorithm 4, Line 39 is delayed to
  /// rescheduleCalls())
  void analyzeExitInst(IntermediateState &IState,
                       DataFlowEnvironment<d_t> &LocalState,
                       FunctionId CurrFunId, ByConstRef<n_t> Inst) {
    const bool InRecursion = IState.InRecursion;
    bool Changed = false;

    auto &Sum = Summaries[CurrFunId].EndSummary;

    for (auto &&[ExitFact, ExitSrc] : LocalState) {
      if constexpr (HasShouldBeInSummary<ProblemT>) {
        if (!Problem->shouldBeInSummary(ExitFact, Inst)) {
          continue;
        }
      }

      auto [It, Inserted] = Sum.try_emplace(ExitFact, std::move(ExitSrc));
      if (InRecursion) {
        Changed |= Inserted || It->second.tryMergeWith(std::move(ExitSrc));
      } else if (!Inserted) {
        It->second.insertAllOf(std::move(ExitSrc));
      }
    }

    if (Changed /* && InRecursion*/) {
      IState.HasNewSummary.insert(CurrFunId);
    }
  }

  /// Procedure AnalyzeCall (Lines 21-34 in Algorithm 4)
  void analyzeCallInst(IntermediateState &IState,
                       DataFlowEnvironment<d_t> &LocalState,
                       FunctionId CurrFunId, ByConstRef<n_t> Inst) {

    const auto &Callees = ICF->getCalleesOfCallAt(Inst);

    auto CSInfo = handleCallSrcSinksAndMayRecurse(IState, LocalState, Callees,
                                                  CurrFunId, Inst);

    if (CSInfo.MayRecurse) {
      IState.InRecursion = true;
      IState.PathEdges[Inst] = LocalState;
    }

    DataFlowEnvironment<d_t> CollectedSummary;

    for (const auto &CalleeFun : Callees) {
      // Collect all data-flows that need to be propagated. Don't update
      // LocalState in-place

      if (IState.LocalProblem.summaryFlow(std::as_const(LocalState),
                                          CollectedSummary, Inst, CalleeFun)) {
        continue;
      }

      auto CalleeId = Functions->get(CalleeFun);
      if (ICF->getStartPointsOf(CalleeFun).empty()) {
        CSInfo.CanCTR = false;
      }
      applySummary(IState, std::as_const(LocalState), CollectedSummary,
                   CalleeId, Inst, CurrFunId);
    }
    if (CSInfo.CanCTR) {
      IState.LocalProblem.callToRetFlow(LocalState, Inst);
    }

    mergeStates(LocalState, std::move(CollectedSummary));
  }

  /// Lines 26-32 in Algorithm 4
  void applySummary(IntermediateState &IState,
                    const DataFlowEnvironment<d_t> &In,
                    DataFlowEnvironment<d_t> &LocalState, FunctionId CalleeId,
                    n_t Inst, FunctionId CurrFunId) {
    const auto &Sum = Summaries[CalleeId];
    Mapper M(Sum.SourceFactIds.size());

    for (const auto &[SumFact, SumSrc] : Sum.EndSummary) {
      auto &&RetFacts = IState.LocalProblem.returnFlow(Inst, SumFact);
      if (RetFacts.empty()) {
        continue;
      }

      const auto &RetSrcFacts =
          M.getAllSrcFactsFor(IState.LocalProblem, In, Sum, SumSrc, Inst);
      if (RetSrcFacts.empty()) {
        continue;
      }

      for (const auto *RetFact : RetFacts) {
        LocalState[RetFact].insertAllOf(RetSrcFacts);
      }
    }

    if (CalleeId != CurrFunId) { // Prevent self-insertion
      for (const auto &[CalleeLeak, LeakSrc] : Sum.LeakIf) {
        const auto &CSSrc =
            M.getAllSrcFactsFor(IState.LocalProblem, In, Sum, LeakSrc, Inst);
        reportOrPropagateLeak(IState, CurrFunId, CalleeLeak.first,
                              CalleeLeak.second, CSSrc);
      }
    }
  }

  struct CallSiteInfo {
    bool MayRecurse = false;
    bool CanCTR = false;
  };

  [[nodiscard]] CallSiteInfo handleCallSrcSinksAndMayRecurse(
      IntermediateState &IState, DataFlowEnvironment<d_t> &LocalState,
      const auto &Callees, FunctionId CurrFunId, ByConstRef<n_t> Inst) {

    const auto &SCCs = *this->SCCs;
    const auto CurrSCC = IState.CurrSCC;

    bool MayRecurse = false;
    bool CanCTR = !Callees.empty();
    for (f_t CalleeFun : Callees) {
      auto CalleeId = Functions->get(CalleeFun);
      auto CalleeSCC = SCCs.SCCOfNode[CalleeId];
      if (CalleeSCC == CurrSCC) {
        MayRecurse = true;
        IState.Incoming[CalleeFun].insert(Inst);
      }

      PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                           "[handleCallSrcSinksAndMayRecurse]: At call to "
                               << FToString(CalleeFun));

      IState.LocalProblem.requestResultCallbackAtCallSite(
          Inst, CalleeFun, [&](ByConstRef<d_t> LeakFact) {
            PHASAR_LOG_LEVEL_CAT(
                DEBUG, LogCategory,
                "[handleCallSrcSinksAndMayRecurse]:   LeakFact: "
                    << DToString(LeakFact));
            if (const auto *LeakSrc = psr::getOrNull(LocalState, LeakFact)) {
              reportOrPropagateLeak(IState, CurrFunId, Inst, LeakFact,
                                    *LeakSrc);
            }
          });

      // Generate taints from zero:
      IState.LocalProblem.generateFactsAtCall(
          Inst, CalleeFun, [&](ByConstRef<d_t> GenFact) {
            PHASAR_LOG_LEVEL_CAT(
                DEBUG, LogCategory,
                "[handleCallSrcSinksAndMayRecurse]:   GenFact: "
                    << DToString(GenFact));
            // Note: Assume, this gets called for all relevant aliases as well
            LocalState[GenFact].insert(SourceFactId(0));
          });
    }

    return {
        .MayRecurse = MayRecurse,
        .CanCTR = CanCTR,
    };
  }

  void handleSourceSinkConfig(IntermediateState &IState,
                              DataFlowEnvironment<d_t> &LocalState,
                              FunctionId CurrFunId, n_t Inst) {
    IState.LocalProblem.requestResultCallback(Inst, [&](const auto &LeakFact) {
      if (const auto *LeakSrc = getOrNull(LocalState, LeakFact)) {
        reportOrPropagateLeak(IState, CurrFunId, Inst, LeakFact, *LeakSrc);
      }
    });

    // Generate taints from zero:
    IState.LocalProblem.generateFacts(Inst, [&](const auto &GenFact) {
      LocalState[GenFact].insert(SourceFactId(0));
    });
  }

  void rescheduleCallsAtExit(IntermediateState &IState, auto &Driver,
                             FunctionId CurrFunId) {
    const auto &Fun = (*Functions)[CurrFunId];
    const auto EnableEnvVersioning = Config.EnableEnvVersioning;

    for (const auto &CS : getOrDefault(IState.Incoming, Fun)) {
      if (auto CallerId = Functions->getOrNull(CS->getFunction())) {
        if (EnableEnvVersioning) {
          IState.PathEdges[CS].Version++;
        }

        Driver.push(CS);
      }
    }
  }

  // PropagateLeaks procedure in Algorithm 4
  void reportOrPropagateLeak(IntermediateState &IState, FunctionId CurrFunId,
                             n_t LeakInst, d_t LeakFact, SourceFactSet From) {
    PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                         "[reportOrPropagateLeak]: " << DToString(LeakFact)
                                                     << " AT "
                                                     << NToString(LeakInst));
    // The zero fact has always Id 0!
    if (From.tryErase(SourceFactId(0))) {
      if (Leaks[LeakInst].insert(LeakFact).second) {
        IState.LocalProblem.onResult(LeakInst, LeakFact);
      }
    }

    auto &CurrSum = Summaries[CurrFunId];

    bool New =
        CurrSum.LeakIf[{LeakInst, LeakFact}].tryMergeWith(std::move(From));

    if (New && IState.InRecursion) {
      IState.HasNewLeaks.insert(CurrFunId);
    }
  }

  static void mergeStates(DataFlowEnvironment<d_t> &Into,
                          DataFlowEnvironment<d_t> &&From) {
    if (Into.empty()) {
      if (&Into != &From) {
        Into = std::move(From);
      }

      return;
    }

    if (Into.size() < From.size()) {
      std::swap(Into, From);
    }

    for (auto &[TgtFact, SrcFactIds] : From) {
      auto [It, Inserted] = Into.try_emplace(TgtFact, std::move(SrcFactIds));
      if (!Inserted) {
        It->second.insertAllOf(std::move(SrcFactIds));
      }
    }
  }

  [[nodiscard]] static bool
  tryMergeStates(DataFlowEnvironment<d_t> &Into,
                 const DataFlowEnvironment<d_t> &From) {
    // TODO Handle phis

    if (Into.empty()) {
      Into = From;
      return !From.empty();
    }

    bool Changed = false;
    for (const auto &[TgtFact, SrcFactIds] : From) {
      auto [It, Inserted] = Into.try_emplace(TgtFact, SrcFactIds);
      Changed |= Inserted || It->second.tryMergeWith(SrcFactIds);
    }

    return Changed;
  }

  // -- data members

  ProblemT *Problem{};
  const i_t *ICF{};

  MonoIfdsConfig Config{};

  std::pmr::monotonic_buffer_resource MBufRes;
  // XXX: Make this synchronized when parallelizing!
  std::pmr::unsynchronized_pool_resource PoolRes{&MBufRes};

  MaybeUniquePtr<const SCCHolder<FunctionId>> SCCs{};
  MaybeUniquePtr<const Compressor<f_t, FunctionId>> Functions{};

  // --- global analysis state
  TypedVector<FunctionId, FunctionSummary> Summaries{};
  llvm::SmallDenseMap<n_t, llvm::SmallDenseSet<d_t, 1>> Leaks{};
};

template <MonoIFDSProblem ProblemT, ICFG ICFGTy>
void MonoIFDSSolver<ProblemT, ICFGTy>::solve() {
  // Step 1: Check for pre-analysis results: If any of them is null, create them
  initialize();

  // Step 2: Pre-allocate buffers
  Summaries.reserve(Functions->size());
  Summaries.append(psr::repeat(&PoolRes, Functions->size()));

  // Step 3: Analyze each CG-SCC in isolation

  for (const auto &[SCC, CurrFuns] : SCCs->NodesInSCC.enumerate()) {
    PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                         "[computeFixpointForSCC]: " << SCC.Value << '/'
                                                     << SCCs->size());
    computeFixpointForSCC(SCC, CurrFuns);
  }
}

} // namespace psr::monoifds
