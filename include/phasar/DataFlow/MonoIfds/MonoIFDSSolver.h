#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel, Eric Bodden.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/CFG.h"
#include "phasar/ControlFlow/ControlFlowOrder.h"
#include "phasar/DataFlow/MonoIfds/ArraySetWorkList.h"
#include "phasar/DataFlow/MonoIfds/DataFlowEnvironment.h"
#include "phasar/DataFlow/MonoIfds/MonoIFDSConfig.h"
#include "phasar/DataFlow/MonoIfds/MonoIFDSProblem.h"
#include "phasar/DataFlow/MonoIfds/RPOWorkList.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/FunctionCompressor.h"
#include "phasar/Utils/Lazy.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/Nullable.h"
#include "phasar/Utils/SCCGeneric.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/UsedGlobalsHolder.h"

#include <concepts>
#include <memory_resource>
#include <unordered_map>
#include <utility>

#include <llvm-16/llvm/Support/Compiler.h>

namespace psr::monoifds {

template <MonoIFDSProblem ProblemT> class MonoIFDSSolver {
public:
  using n_t = typename ProblemT::ProblemAnalysisDomain::n_t;
  using d_t = typename ProblemT::ProblemAnalysisDomain::d_t;
  using i_t = typename ProblemT::ProblemAnalysisDomain::i_t;
  using f_t = typename ProblemT::ProblemAnalysisDomain::f_t;
  using v_t = typename ProblemT::ProblemAnalysisDomain::v_t;

  explicit MonoIFDSSolver(ProblemT *Problem, const i_t *ICF,
                          std::pmr::polymorphic_allocator<> Alloc =
                              std::pmr::get_default_resource())
      : Problem(Problem), ICF(ICF), MBufRes(Alloc.resource()) {}

  MonoIFDSSolver &setConfig(MonoIfdsConfig Config) & noexcept {
    this->Config = Config;
    return *this;
  }

  MonoIFDSSolver &setCGSCCs(const SCCHolder<FunctionId> *SCCs) & noexcept {
    this->SCCs = SCCs;
    return *this;
  }

  MonoIFDSSolver
  setFunctionCompressor(const FunctionCompressor *Functions) & noexcept {
    this->Functions = Functions;
    return *this;
  }

  MonoIFDSSolver &
  setUsedGlobals(const UsedGlobalsHolder<v_t> *UsedGlobals) & noexcept {
    this->UsedGlobals = UsedGlobals;
    return *this;
  }

  void solve();

private:
  // NOTE: Used the node_hash_map from
  // [parallel-hash-map](https://github.com/greg7mdp/parallel-hashmap) here
  // for the paper-eval!
  template <typename Key, typename Value>
  using node_hash_map = std::pmr::unordered_map<Key, Value>;

  struct FunctionSummary {
    Compressor<d_t, SourceFactId> SourceFactIds;
    DataFlowEnvironment<d_t> EndSummary;

    [[clang::require_explicit_initialization]] node_hash_map<
        std::pair<n_t, d_t>, SourceFactSet> LeakIf;
  };

  struct IntermediateState {
    node_hash_map<n_t, DataFlowEnvironment<d_t>> PathEdges;
    node_hash_map<f_t, llvm::SmallDenseSet<n_t>> Incoming;

    llvm::SmallDenseSet<FunctionId> HasNewLeaks;
    llvm::SmallDenseSet<FunctionId> HasNewSummary;

    std::reference_wrapper<
        const llvm::SmallDenseSet<const llvm::GlobalVariable *>>
        PermittedGlobals;
    SCCId<FunctionId> CurrSCC;
    bool InRecursion;

    IntermediateState(std::pmr::memory_resource *MRes,
                      const UsedGlobalsHolder<v_t> &UsedGlobals,
                      SCCId<FunctionId> CurrSCC, bool InRecursion)
        : PathEdges(MRes), Incoming(MRes),
          PermittedGlobals(std::cref(UsedGlobals.GlobsPerSCC[CurrSCC])),
          CurrSCC(CurrSCC), InRecursion(InRecursion) {}
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

    const SourceFactSet &getSourceFactsFor(auto &Solver,
                                           const DataFlowEnvironment<d_t> &In,
                                           const FunctionSummary &CalleeSum,
                                           SourceFactId CalleeSrc,
                                           ByConstRef<n_t> CallInst) {
      auto &Ret = Mapping[CalleeSrc];

      if (ComputedMappings.tryInsert(CalleeSrc)) {
        auto &&CSFacts = Solver.Problem->invReturnFlow(
            CallInst, CalleeSum.SourceFactIds[CalleeSrc]);

        for (const auto *Fact : CSFacts) {
          if (const auto *FactSrc = getOrNull(In, Fact)) {
            Ret.insertAllOf(*FactSrc);
          }
        }
      }

      return Ret;
    }

    void insertAllSrcFactsFor(SourceFactSet &Into, auto &Solver,
                              const DataFlowEnvironment<d_t> &In,
                              const FunctionSummary &CalleeSum,
                              const SourceFactSet &CalleeSrcs,
                              ByConstRef<n_t> CallInst) {
      CalleeSrcs.foreach ([&](auto SrcFactId) {
        Into.insertAllOf(
            getSourceFactsFor(Solver, In, CalleeSum, SrcFactId, CallInst));
      });
    }

    [[nodiscard]] SourceFactSet
    getAllSrcFactsFor(auto &Solver, const DataFlowEnvironment<d_t> &In,
                      const FunctionSummary &CalleeSum,
                      const SourceFactSet &CalleeSrcs,
                      ByConstRef<n_t> CallInst) {
      SourceFactSet Ret;
      insertAllSrcFactsFor(Ret, Solver, In, CalleeSum, CalleeSrcs, CallInst);
      return Ret;
    }
  };

  void computeFixpointForSCC(SCCId<FunctionId> CurrSCC,
                             llvm::ArrayRef<FunctionId> CurrFuns) {
    const size_t SCCSize = CurrFuns.size();
    const bool InRecursion = SCCSize > 1;
    IntermediateState IState(&PoolRes, *UsedGlobals, CurrSCC, InRecursion);

    const auto IterStrategy = Config.IterStrategy;
    const bool UseTopoFixpointDriver = [=] {
      if (IterStrategy == IterationStrategy::DedupFIFOQueue) {
        return false;
      }

      if (IterStrategy == IterationStrategy::HybridCapped) {
        // return SCCSize < 20;
        return SCCSize == 1;
      }

      return true;
    }();

    ControlFlowOrder CFO;
    if (UseTopoFixpointDriver) {
      // TODO: implement computeCFGOrder()
      computeCFGOrder(CFO, SCCs, CurrSCC, *ICF, Functions);
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

          llvm::errs() << '.';
        };

    const auto RepropagateInRecursion = [&](auto &Driver) {
      rescheduleCalls(IState, Driver, SCCs, CurrSCC, Functions);
      while (!Driver.empty()) {
        Driver.run(
            [&](n_t BlockStart) { analyzeBlock(IState, Driver, BlockStart); });
        assert(Driver.empty());

        rescheduleCalls(IState, Driver, SCCs, CurrSCC, Functions);
        llvm::errs() << '.';
      }

      ITST_ASSERT(IState.HasNewSummary.empty(),
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

    repropagateLeaks(IState, SCCs, CurrSCC, Functions);
  }

  void submitInitialSeeds(IntermediateState &IState, auto &Driver,
                          Compressor<d_t, SourceFactId> &SeedCompressor,
                          ByConstRef<f_t> Fun) {
    const auto &SPs = ICF->getStartPointsOf(Fun);

    const auto &Zero = Problem->getZeroValue();
    SeedCompressor.insert(Zero);
    assert(SeedCompressor.get(Zero) == SourceFactId(0) &&
           "The Zero value must always have Id 0!");

    for (const auto &SP : SPs) {
      auto &SeedState = IState.PathEdges[SP];
      SeedState[Zero].insert(SourceFactId(0));

      Problem->initialSeeds(SeedState, Fun);
      Driver.push(SP);
    }
  }

  void rescheduleCalls(IntermediateState &IState, auto &Driver) {
    if (!IState.InRecursion) {
      return;
    }

    const bool EnableEnvVersioning = Config.EnableEnvVersioning;

    for (auto FunId : IState.HasNewSummary) {
      IState.HasNewLeaks.erase(FunId);
      const auto &Fun = Functions[FunId];

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

  void repropagateLeaks(IntermediateState &IState, SCCId<FunctionId> CurrSCC) {
    llvm::SmallDenseSet<FunctionId> NewLeaksWL;
    while (!IState.HasNewLeaks.empty()) {
      NewLeaksWL.swap(IState.HasNewLeaks);

      for (auto FunId : NewLeaksWL) {
        handleLeaksForFun(IState, SCCs, CurrSCC, Functions, FunId);
      }
      NewLeaksWL.clear();
    }
  }

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
        const auto &CSSrc = M.getAllSrcFactsFor(*this, In, Sum, LeakSrc, CS);
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

  void analyzeBlockImpl(IntermediateState &IState, auto &Driver,
                        ByConstRef<n_t> BlockStart,
                        DataFlowEnvironment<d_t> LocalState) {

    auto CurrFunId = Functions.get(BlockStart->getFunction());

    // const bool EnableAggressiveLoopPriorization =
    //     Config.EnableAggressiveLoopPriorization;

    Nullable<n_t> CurrInst = BlockStart;

    do {
      auto Last = CurrInst;

      do {
        analyzeInstruction(IState, LocalState, CurrFunId,
                           unwrapNullable(CurrInst));
        Last = CurrInst;
        if constexpr (IsBlockAwareControlFlow<i_t>) {
          CurrInst = ICF->getUniqueSuccessor(unwrapNullable(CurrInst));
        } else {
          const auto &Succs = ICF->getSuccsOf(unwrapNullable(CurrInst));
          if (Succs.size() == 1) {
            CurrInst = Succs[0];
          } else {
            CurrInst = {};
          }
        }
      } while (CurrInst);

      Nullable<n_t> UniqueSucc{};

      // We have at least one instruction, so we can safely unwrap here
      const auto &Succs = ICF->getSuccsOf(unwrapNullable(Last));
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

          // note: HasSingleSucc implies here that UniqueSucc==nullptr

          // TODO: Should we support EnableAggressiveLoopPriorization outside of
          // LLVM? It did not show significant performance benefits, though

          // if (EnableAggressiveLoopPriorization && HasSingleSucc &&
          //     Block->getTerminator()->hasMetadata(llvm::LLVMContext::MD_loop))
          //     {
          //   UniqueSucc = Succ;
          // } else {
          Driver.push(Succ);
          // }
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

    Problem->normalFlow(LocalState, Inst);
  }

  void analyzeExitInst(IntermediateState &IState,
                       DataFlowEnvironment<d_t> &LocalState,
                       FunctionId CurrFunId, ByConstRef<n_t> Inst) {
    const bool InRecursion = IState.InRecursion;
    bool Changed = false;

    auto &Sum = Summaries[CurrFunId].EndSummary;

    for (auto &&[ExitFact, ExitSrc] : LocalState) {
      if constexpr (requires(ProblemT &P) {
                      {
                        P.shouldBeInSummary(ExitFact, Inst)
                      } -> std::convertible_to<bool>;
                    }) {
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

  void analyzeCallInst(IntermediateState &IState,
                       DataFlowEnvironment<d_t> &LocalState,
                       FunctionId CurrFunId, ByConstRef<n_t> Inst) {

    const auto &Callees = ICF->getCalleesOfCallAt(Inst);

    const auto CSInfo = handleCallSrcSinksAndMayRecurse(
        IState, LocalState, Callees, CurrFunId, Inst);

    if (CSInfo.MayRecurse) {
      IState.InRecursion = true;
      IState.PathEdges[Inst] = LocalState;
    }

    DataFlowEnvironment<d_t> CollectedSummary;

    for (const auto &CalleeFun : Callees) {
      // Collect all data-flows that need to be propagated. Don't update
      // LocalState in-place

      auto CalleeId = Functions.get(CalleeFun);
      applySummary(IState, std::as_const(LocalState), CollectedSummary,
                   CalleeFun, CalleeId, Inst, CurrFunId);
    }
    if (CSInfo.CanCTR) {
      Problem->callToRetFlow(LocalState, Inst);
    }

    mergeStates(LocalState, std::move(CollectedSummary));
  }

  // TODO: applySummary
  // TODO: handleCallSrcSinksAndMayRecurse
  // TODO: tryMergeStates, mergeStates
  // TODO: rescheduleCallsAtExit
  // TODO: reportOrPropagateLeak

  // TODO: Add srcsink-config to MonoIFDSProblem

  // -- data members

  ProblemT *Problem{};
  const i_t *ICF{};

  MonoIfdsConfig Config{};

  std::pmr::monotonic_buffer_resource MBufRes;
  // XXX: Make this synchronized when parallelizing!
  std::pmr::unsynchronized_pool_resource PoolRes{&MBufRes};

  MaybeUniquePtr<const SCCHolder<FunctionId>> SCCs{};
  MaybeUniquePtr<const FunctionCompressor> Functions{};
  MaybeUniquePtr<const UsedGlobalsHolder<v_t>> UsedGlobals{};

  // --- global analysis state
  TypedVector<FunctionId, FunctionSummary> Summaries{};
  llvm::SmallDenseMap<n_t, llvm::SmallDenseSet<d_t, 1>> Leaks{};
};

template <MonoIFDSProblem ProblemT> void MonoIFDSSolver<ProblemT>::solve() {
  // Step 1: Check for pre-analysis results: If any of them is null, create them

  // Step 2: Pre-allocate buffers
  Summaries.resize(Functions->size());

  // Step 3: Analyze each CG-SCC in isolation

  for (const auto &[SCC, CurrFuns] : SCCs->NodesInSCC.enumerate()) {
    computeFixpointForSCC(SCC, CurrFuns);
  }
}

} // namespace psr::monoifds
