#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_ITERATIVEIDESOLVER_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_ITERATIVEIDESOLVER_H

#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"
#include "phasar/DataFlow/IfdsIde/Solver/Compressor.h"
#include "phasar/DataFlow/IfdsIde/Solver/EdgeFunctionCache.h"
#include "phasar/DataFlow/IfdsIde/Solver/FlowEdgeFunctionCacheNG.h"
#include "phasar/DataFlow/IfdsIde/Solver/FlowFunctionCache.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolverAPIMixin.h"
#include "phasar/DataFlow/IfdsIde/Solver/IdBasedSolverResults.h"
#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolverBase.h"
#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolverResults.h"
#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolverStats.h"
#include "phasar/DataFlow/IfdsIde/Solver/StaticIDESolverConfig.h"
#include "phasar/DataFlow/IfdsIde/Solver/WorkListTraits.h"
#include "phasar/DataFlow/IfdsIde/SolverResults.h"
#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/EmptyBaseOptimizationUtils.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/StableVector.h"
#include "phasar/Utils/TableWrappers.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

namespace psr {

/// Solves the given IDETabulationProblem as described in the 1996 paper by
/// Sagiv, Horwitz and Reps. To solve the problem, call solve().
///
/// This solver implements the optimizations and the $JF_N$ layout from the
/// paper "Scaling Interprocedural Static Data-Flow Analysis to Large C/C++
/// Applications: An Experience Report"
/// (<https://doi.org/10.4230/LIPIcs.ECOOP.2024.36>) by Schiebel, Sattler,
/// Schubert, Apel, and Bodden.
template <typename ProblemTy,
          typename StaticSolverConfigTy = DefaultIDESolverConfig<ProblemTy>,
          ICFG ICFGTy = typename ProblemTy::ProblemAnalysisDomain::i_t>
class IterativeIDESolver
    : private SolverStatsSelector<StaticSolverConfigTy>,
      private detail::IterativeIDESolverResults<
          typename ProblemTy::ProblemAnalysisDomain::n_t,
          typename ProblemTy::ProblemAnalysisDomain::d_t,
          std::conditional_t<StaticSolverConfigTy::ComputeValues,
                             typename ProblemTy::ProblemAnalysisDomain::l_t,
                             BinaryDomain>>,
      public IterativeIDESolverBase<
          StaticSolverConfigTy,
          typename StaticSolverConfigTy::template EdgeFunctionPtrType<
              typename ProblemTy::ProblemAnalysisDomain::l_t>>,
      public IDESolverAPIMixin<
          IterativeIDESolver<ProblemTy, StaticSolverConfigTy, ICFGTy>> {

  friend IDESolverAPIMixin<
      IterativeIDESolver<ProblemTy, StaticSolverConfigTy, ICFGTy>>;

public:
  using domain_t = typename ProblemTy::ProblemAnalysisDomain;
  using d_t = typename domain_t::d_t;
  using n_t = typename domain_t::n_t;
  using f_t = typename domain_t::f_t;
  using t_t = typename domain_t::t_t;
  using v_t = typename domain_t::v_t;
  using l_t = std::conditional_t<StaticSolverConfigTy::ComputeValues,
                                 typename domain_t::l_t, BinaryDomain>;
  using i_t = ICFGTy;

  using config_t = StaticSolverConfigTy;

private:
  using base_t = IterativeIDESolverBase<
      StaticSolverConfigTy,
      typename StaticSolverConfigTy::template EdgeFunctionPtrType<
          typename domain_t::l_t>>;

  using base_results_t = detail::IterativeIDESolverResults<n_t, d_t, l_t>;

  using base_t::ComputeValues;
  using base_t::EnableStatistics;
  static constexpr bool UseEndSummaryTab = config_t::UseEndSummaryTab;
  using typename base_t::EdgeFunctionPtrType;
  using typename base_t::InterPropagationJob;
  using typename base_t::InterPropagationJobRef;
  using typename base_t::InterPropagationJobRefDSI;
  using typename base_t::PropagationJob;
  using typename base_t::SummaryEdge;
  using typename base_t::SummaryEdges;
  using typename base_t::SummaryEdges_JF1;
  using typename base_t::ValuePropagationJob;

  using base_results_t::FactCompressor;
  using base_results_t::NodeCompressor;
  using base_results_t::ValCompressor;

  template <typename K, typename V>
  using map_t = typename base_t::template map_t<K, V>;

  template <typename T> using set_t = typename base_t::template set_t<T>;

  template <typename T>
  using worklist_t = typename base_t::template worklist_t<T>;

  using flow_edge_function_cache_t =
      typename base_t::template flow_edge_function_cache_t<
          ProblemTy, StaticSolverConfigTy::AutoAddZero>;

  using typename base_t::summaries_t;

  static constexpr JumpFunctionGCMode EnableJumpFunctionGC =
      StaticSolverConfigTy::EnableJumpFunctionGC;

public:
  IterativeIDESolver(ProblemTy *Problem, const ICFGTy *ICFG,
                     StaticSolverConfigTy /*Config*/ = {}) noexcept
      : Problem(assertNotNull(Problem)), ICFG(assertNotNull(ICFG)) {}

  auto solve() & {
    solveImpl();
    return getSolverResults();
  }
  [[nodiscard]] auto solve() && {
    solveImpl();
    return consumeSolverResults();
  }

  [[nodiscard]] auto getSolverResults() const noexcept {
    return IdBasedSolverResults<n_t, d_t, l_t>(this);
  }

  [[nodiscard]] auto consumeSolverResults() noexcept {
    return OwningIdBasedSolverResults<n_t, d_t, l_t>(
        std::make_unique<base_results_t>(
            std::move(static_cast<base_results_t &&>(*this))));
  }

  void dumpResults(llvm::raw_ostream &OS = llvm::outs()) const {
    getSolverResults().dumpResults(ICFG, OS);
  }

  [[nodiscard]] IterativeIDESolverStats getStats() const noexcept
    requires EnableStatistics
  {
    return *this;
  }

  void dumpStats(llvm::raw_ostream &OS = llvm::outs()) const
    requires EnableStatistics
  {
    OS << getStats();
  }

private:
  void doInitialize() {
    const auto NumInsts = Problem.getProjectIRDB()->getNumInstructions();
    const auto NumFuns = Problem.getProjectIRDB()->getNumFunctions();

    NodeCompressor =
        NodeCompressorTraits<n_t>::create(Problem.getProjectIRDB());

    JumpFunctions.reserve(NumInsts);
    this->base_results_t::ValTab.reserve(NumInsts);

    /// Initial size of 64 is too much for jump functions per instruction; 16
    /// should be better:
    for (size_t I = 0; I != NumInsts; ++I) {
      JumpFunctions.emplace_back();
      this->base_results_t::ValTab.emplace_back().reserve(16);
    }
    if constexpr (EnableJumpFunctionGC != JumpFunctionGCMode::Disabled) {
      RefCountPerFunction = llvm::OwningArrayRef<size_t>(NumFuns);
      std::uninitialized_fill_n(RefCountPerFunction.data(),
                                RefCountPerFunction.size(), 0);
      CandidateFunctionsForGC.resize(NumFuns);
    }

    NodeCompressor.reserve(NumInsts);
    FactCompressor.reserve(NumInsts);
    FunCompressor.reserve(NumFuns);
    FECache.reserve(NumInsts, ICFG.getNumCallSites(), NumFuns);
    /// Make sure, that the Zero-flowfact always has the ID 0
    FactCompressor.getOrInsert(Problem.getZeroValue());

    submitInitialSeeds();
  }

  bool doNext() {
    std::optional Job = WorkList.pop();
    if (!Job) [[unlikely]] {
      processInterJobs();
      if constexpr (EnableJumpFunctionGC != JumpFunctionGCMode::Disabled) {
        /// CAUTION: The functions from the CallWL also need to be considered
        /// live! We therefore need to be careful when applying this GC in a
        /// multithreaded environment

        runGC();
      }

      Job = WorkList.pop();
      if (!Job) {
        return false;
      }
    }

    propagate(Job->AtInstruction, Job->SourceFact, Job->PropagatedFact,
              std::move(Job->SourceEF));

    return true;
  }

  void finalizePhase1() {
    if constexpr (EnableStatistics) {
      this->FEStats = FECache.getStats();
      this->NumAllInterPropagations = AllInterPropagations.size();
      this->AllInterPropagationsBytes =
          AllInterPropagations.getApproxSizeInBytes() +
          AllInterPropagationsOwner.getApproxSizeInBytes();
      this->SourceFactAndCSToInterJobSize = SourceFactAndCSToInterJob.size();
      this->SourceFactAndCSToInterJobBytes =
          SourceFactAndCSToInterJob.getApproxSizeInBytes();
      this->SourceFactAndFuncToInterJobSize =
          SourceFactAndFuncToInterJob.size();
      this->SourceFactAndFuncToInterJobBytes =
          SourceFactAndFuncToInterJob.getApproxSizeInBytes();
      this->NumFlowFacts = FactCompressor.size();
      this->InstCompressorCapacity = NodeCompressor.capacity();
      this->FactCompressorCapacity = FactCompressor.capacity();
      this->FunCompressorCapacity = FunCompressor.capacity();

      this->JumpFunctionsMapBytes = JumpFunctions.capacity_in_bytes();
      this->CumulESGEdges = 0;
      this->MaxESGEdgesPerInst = 0;
      for (const auto &JF : JumpFunctions) {
        this->JumpFunctionsMapBytes += JF.getApproxSizeInBytes();
        this->CumulESGEdges += JF.size();
        this->MaxESGEdgesPerInst =
            std::max(this->MaxESGEdgesPerInst, JF.size());
      }

      this->ValTabBytes = this->base_results_t::ValTab.capacity_in_bytes();
      for (const auto &FactsVals : this->base_results_t::ValTab) {
        this->ValTabBytes += FactsVals.getApproxSizeInBytes();
      }

      this->AvgESGEdgesPerInst =
          double(this->CumulESGEdges) / JumpFunctions.size();

      if constexpr (UseEndSummaryTab) {
        this->EndSummaryTabSize = EndSummaryTab.getApproxSizeInBytes();
        this->NumEndSummaries = 0;
        for (const auto &Summary : EndSummaryTab.cells()) {
          this->NumEndSummaries += Summary.second.size();
          this->EndSummaryTabSize += Summary.second.getMemorySize();
        }
      }
    }

    FECache.clearFlowFunctions();
    SourceFactAndFuncToInterJob.clear();
    WorkList.clear();
    CallWL.clear();
  }

  void doFinalizeImpl() {
    finalizePhase1();
    /// Finished Phase I, now go for Phase II if necessary
    performValuePropagation();
  }

  auto doFinalize() & {
    doFinalizeImpl();
    return getSolverResults();
  }

  auto doFinalize() && {
    doFinalizeImpl();
    return consumeSolverResults();
  }

  void solveImpl() {
    doInitialize();

    performDataflowFactPropagation();

    /// Finished Phase I, now go for Phase II if necessary
    performValuePropagation();
  }

  void performDataflowFactPropagation() {
    std::atomic_bool Finished = true;
    do {
      /// NOTE: Have a separate function on the worklist to process it, to
      /// allow for easier integration with task-pools
      WorkList.processEntriesUntilEmpty([this, &Finished](PropagationJob Job) {
        /// propagate only handles intra-edges as of now - add separate
        /// functionality to handle inter-edges as well
        propagate(Job.AtInstruction, Job.SourceFact, Job.PropagatedFact,
                  std::move(Job.SourceEF));
        bool Dummy = true;
        Finished.compare_exchange_strong(
            Dummy, false, std::memory_order_release, std::memory_order_relaxed);
      });

#ifndef NDEBUG
      // Sanity checks
      if (llvm::any_of(RefCountPerFunction, [](auto RC) { return RC != 0; })) {
        llvm::report_fatal_error(
            "Worklist.empty() does not imply Function ref-counts==0 ?");
      }

      if (!WorkList.empty()) {
        llvm::report_fatal_error(
            "Worklist should be empty after processing all items");
      }
#endif // NDEBUG

      assert(WorkList.empty() &&
             "Worklist should be empty after processing all items");

      processInterJobs();

      if constexpr (EnableJumpFunctionGC != JumpFunctionGCMode::Disabled) {
        /// CAUTION: The functions from the CallWL also need to be considered
        /// live! We therefore need to be careful when applying this GC in a
        /// multithreaded environment

        runGC();
      }

    } while (Finished.exchange(true, std::memory_order_acq_rel) == false);

    finalizePhase1();
  }

  void performValuePropagation() {
    if constexpr (ComputeValues) {
      /// NOTE: We can already clear the EFCache here, as we are not querying
      /// any edge function in Phase II; The EFs that are in use are kept alive
      /// by their shared_ptr
      FECache.clear();

      submitInitialValues();
      WLProp.processEntriesUntilEmpty([this](ValuePropagationJob Job) {
        propagateValue(Job.Inst, Job.Fact, std::move(Job.Value));
      });

      WLProp.clear();

      for (uint32_t SP : WLComp) {
        computeValues(SP);
      }

      if constexpr (EnableStatistics) {
        this->WLCompHighWatermark = WLComp.size();
      }

      WLComp.clear();
    }

    JumpFunctions.clear();
    SourceFactAndCSToInterJob.clear();
    AllInterPropagations.clear();
    AllInterPropagationsOwner.clear();
  }

  void submitInitialSeeds() {
    auto Seeds = Problem.initialSeeds();
    EdgeFunctionPtrType IdFun = [] {
      if constexpr (ComputeValues) {
        return EdgeIdentity<l_t>{};
      } else {
        return EdgeFunctionPtrType{};
      }
    }();

    for (const auto &[Inst, SeedMap] : Seeds.getSeeds()) {
      auto InstId = NodeCompressor.getOrInsert(Inst);
      auto Fun = FunCompressor.getOrInsert(ICFG.getFunctionOf(Inst));
      for (const auto &[Fact, Val] : SeedMap) {
        auto FactId = FactCompressor.getOrInsert(Fact);
        auto &JumpFns = JumpFunctions[InstId];

        storeResultsAndPropagate(JumpFns, InstId, FactId, FactId, Fun, IdFun);
      }
    }
  }

  template <bool Set>
  [[nodiscard]] bool
  keepAnalysisInformationAt(ByConstRef<n_t> Inst) const noexcept {
    if (ICFG.isExitInst(Inst) || ICFG.isStartPoint(Inst)) {
      /// Keep the procedure summaries + starting points, such that
      /// already analyzed paths are not analyzed again
      return true;
    }

    if (llvm::any_of(ICFG.getPredsOf(Inst), [ICF{&ICFG}](ByConstRef<n_t> Pred) {
          return ICF->isCallSite(Pred);
        })) {
      /// Keep the return-sites
      return true;
    }

    if constexpr (has_isInteresting_v<ProblemTy> &&
                  (Set || (!Set && ComputeValues))) {
      if (Problem.isInteresting(Inst)) {
        /// Keep analysis information at interesting instructions
        return true;
      }
    }
    return false;
  }

  ByConstRef<l_t> get(uint32_t Inst, uint32_t Fact) {
    return ValCompressor[this->base_results_t::ValTab[size_t(Inst)].getOrCreate(
        Fact)];
  }

  void set(uint32_t Inst, uint32_t Fact, l_t Val) {
    if constexpr (EnableJumpFunctionGC ==
                  JumpFunctionGCMode::EnabledAggressively) {
      if (!keepAnalysisInformationAt<true>(NodeCompressor[Inst])) {
        return;
      }
    }

    auto &Dest = this->base_results_t::ValTab[size_t(Inst)].getOrCreate(Fact);
    if constexpr (!std::is_const_v<std::remove_reference_t<decltype(Dest)>>) {
      Dest = ValCompressor.getOrInsert(std::move(Val));
    }
  }

  bool storeResultsAndPropagate(SummaryEdges &JumpFns, uint32_t SuccId,
                                uint32_t SourceFact, uint32_t LocalFact,
                                uint32_t FunId, EdgeFunctionPtrType LocalEF)
    requires ComputeValues
  {

    if (llvm::isa<AllTop<l_t>>(LocalEF)) {
      // Don't store the default edge-function, which essentially denotes a
      // killed fact
      return false;
    }

    auto &EF = JumpFns.getOrCreate(combineIds(SourceFact, LocalFact));
    if (!EF) {
      EF = std::move(LocalEF);
      /// Register new propagation job as we haven't seen this
      /// fact here yet

      WorkList.emplace(PropagationJob{EF, SuccId, SourceFact, LocalFact});

      set(SuccId, LocalFact, Problem.topElement());

      if constexpr (EnableJumpFunctionGC != JumpFunctionGCMode::Disabled) {
        RefCountPerFunction[FunId]++;
        CandidateFunctionsForGC.set(FunId);
      }

      if constexpr (EnableStatistics) {
        this->NumPathEdges++;
        if (this->NumPathEdges > this->NumPathEdgesHighWatermark) {
          this->NumPathEdgesHighWatermark = this->NumPathEdges;
        }
        if (WorkList.size() > this->WorkListHighWatermark) {
          this->WorkListHighWatermark = WorkList.size();
        }
      }
      return true;
    }

    auto NewEF = Problem.combine(EF, std::move(LocalEF));
    assert(NewEF != nullptr);

    if (NewEF != EF) {
      /// Register new propagation job as we have refined the
      /// edge-function
      EF = NewEF;
      WorkList.emplace(
          PropagationJob{std::move(NewEF), SuccId, SourceFact, LocalFact});

      if constexpr (EnableJumpFunctionGC != JumpFunctionGCMode::Disabled) {
        RefCountPerFunction[FunId]++;
        CandidateFunctionsForGC.set(FunId);
      }
      return true;
    }
    return false;
  }

  bool storeResultsAndPropagate(SummaryEdges &JumpFns, uint32_t SuccId,
                                uint32_t SourceFact, uint32_t LocalFact,
                                uint32_t FunId, EdgeFunctionPtrType /*LocalEF*/)
    requires(!ComputeValues)
  {
    if (JumpFns.insert(combineIds(SourceFact, LocalFact)).second) {
      WorkList.emplace(PropagationJob{{}, SuccId, SourceFact, LocalFact});

      set(SuccId, LocalFact, BinaryDomain::TOP);

      if constexpr (EnableJumpFunctionGC != JumpFunctionGCMode::Disabled) {
        RefCountPerFunction[FunId]++;

        CandidateFunctionsForGC.set(FunId);
      }
      if constexpr (EnableStatistics) {
        this->NumPathEdges++;
        if (this->NumPathEdges > this->NumPathEdgesHighWatermark) {
          this->NumPathEdgesHighWatermark = this->NumPathEdges;
        }
        if (WorkList.size() > this->WorkListHighWatermark) {
          this->WorkListHighWatermark = WorkList.size();
        }
      }
      return true;
    }
    return false;
  }

  void storeSummary(SummaryEdges_JF1 &JumpFns, uint32_t LocalFact,
                    EdgeFunctionPtrType LocalEF)
    requires(UseEndSummaryTab && ComputeValues)
  {
    auto &EF = JumpFns[LocalFact];
    if (!EF) {
      EF = std::move(LocalEF);
      /// Register new propagation job as we haven't seen this
      /// fact here yet

      if constexpr (EnableStatistics) {
        this->NumPathEdges++;
        if (this->NumPathEdges > this->NumPathEdgesHighWatermark) {
          this->NumPathEdgesHighWatermark = this->NumPathEdges;
        }
      }
      return;
    }

    auto NewEF = Problem.combine(EF, std::move(LocalEF));
    assert(NewEF != nullptr);

    if (NewEF != EF) {
      /// Register new propagation job as we have refined the
      /// edge-function
      EF = NewEF;
    }
  }

  void storeSummary(SummaryEdges_JF1 &JumpFns, uint32_t LocalFact,
                    EdgeFunctionPtrType /*LocalEF*/)
    requires(UseEndSummaryTab && !ComputeValues)
  {
    if (JumpFns.insert({LocalFact, {}}).second) {
      if constexpr (EnableStatistics) {
        this->NumPathEdges++;
        if (this->NumPathEdges > this->NumPathEdgesHighWatermark) {
          this->NumPathEdgesHighWatermark = this->NumPathEdges;
        }
      }
    }
  }

  void propagate(uint32_t AtInstructionId, uint32_t SourceFact,
                 uint32_t PropagatedFact, EdgeFunctionPtrType SourceEF) {

    auto AtInstruction = NodeCompressor[AtInstructionId];

    auto FunId = [&] {
      if constexpr (EnableJumpFunctionGC != JumpFunctionGCMode::Disabled) {

        auto Ret = FunCompressor.getOrInsert(ICFG.getFunctionOf(AtInstruction));

        assert(RefCountPerFunction[Ret] > 0);
        RefCountPerFunction[Ret]--;
        return Ret;
      } else {
        (void)this;
        (void)AtInstruction;
        return 0;
      }
    }();

    applyFlowFunction(AtInstruction, AtInstructionId, SourceFact,
                      PropagatedFact, FunId, std::move(SourceEF));
  }

  void applyFlowFunction(ByConstRef<n_t> AtInstruction,
                         uint32_t AtInstructionId, uint32_t SourceFactId,
                         uint32_t PropagatedFactId, uint32_t FunId,
                         EdgeFunctionPtrType SourceEF) {

    if (!ICFG.isCallSite(AtInstruction)) {
      applyNormalFlow(AtInstruction, AtInstructionId, SourceFactId,
                      PropagatedFactId, std::move(SourceEF), FunId);
    } else {
      applyIntraCallFlow(AtInstruction, AtInstructionId, SourceFactId,
                         PropagatedFactId, std::move(SourceEF), FunId);
    }
  }

  void applyNormalFlow(ByConstRef<n_t> AtInstruction, uint32_t AtInstructionId,
                       uint32_t SourceFactId, uint32_t PropagatedFactId,
                       EdgeFunctionPtrType SourceEF, uint32_t FunId) {

    auto CSFact = FactCompressor[PropagatedFactId];

    for (ByConstRef<n_t> Succ : ICFG.getSuccsOf(AtInstruction)) {
      auto SuccId = NodeCompressor.getOrInsert(Succ);
      auto Facts =
          FECache
              .getNormalFlowFunction(Problem, AtInstruction, Succ,
                                     combineIds(AtInstructionId, SuccId))
              .computeTargets(CSFact);

      auto &JumpFns = JumpFunctions[SuccId];

      for (ByConstRef<d_t> Fact : Facts) {
        auto FactId = FactCompressor.getOrInsert(Fact);
        auto EF = [&] {
          if constexpr (ComputeValues) {
            auto NEF = FECache.getNormalEdgeFunction(
                Problem, AtInstruction, CSFact, Succ, Fact,
                combineIds(AtInstructionId, SuccId),
                combineIds(PropagatedFactId, FactId));
            return Problem.extend(SourceEF, std::move(NEF));
          } else {
            return EdgeFunctionPtrType{};
          }
        }();

        storeResultsAndPropagate(JumpFns, SuccId, SourceFactId, FactId, FunId,
                                 std::move(EF));
      }
    }

    /// NOTE: Is isExitInst, we did not enter the above loop. So, we can only
    /// have reached here, if the PropagatedFact (or at least the SourceEF) was
    /// new at AtInstruction.
    if (ICFG.isExitInst(AtInstruction)) {
      if constexpr (EnableJumpFunctionGC == JumpFunctionGCMode::Disabled) {
        auto Fun = ICFG.getFunctionOf(AtInstruction);
        FunId = FunCompressor.getOrInsert(std::move(Fun));
      }

      /// These call-sites might have already been processed, but since we have
      /// now new summary information, we must reschedule them all for
      /// processing in Step3
      CallWL.insert(combineIds(SourceFactId, FunId));

      if constexpr (UseEndSummaryTab) {
        storeSummary(EndSummaryTab.getOrCreate(combineIds(FunId, SourceFactId)),
                     PropagatedFactId, std::move(SourceEF));
      }
    }
  }

  void applyIntraCallFlow(ByConstRef<n_t> AtInstruction,
                          uint32_t AtInstructionId, uint32_t SourceFactId,
                          uint32_t PropagatedFactId,
                          EdgeFunctionPtrType SourceEF, uint32_t FunId) {
    const auto &Callees = ICFG.getCalleesOfCallAt(AtInstruction);

    applyCallToReturnFlow(AtInstruction, AtInstructionId, SourceFactId,
                          PropagatedFactId, SourceEF, Callees, FunId);

    handleOrDeferCallFlow(AtInstruction, AtInstructionId, SourceFactId,
                          PropagatedFactId, std::move(SourceEF), Callees,
                          FunId);
  }

  template <typename CalleesTy>
  void applyCallToReturnFlow(ByConstRef<n_t> AtInstruction,
                             uint32_t AtInstructionId, uint32_t SourceFactId,
                             uint32_t PropagatedFactId,
                             EdgeFunctionPtrType SourceEF,
                             const CalleesTy &Callees, uint32_t FunId) {
    auto CSFact = FactCompressor[PropagatedFactId];

    for (ByConstRef<n_t> RetSite : ICFG.getReturnSitesOfCallAt(AtInstruction)) {
      auto RetSiteId = NodeCompressor.getOrInsert(RetSite);
      auto Facts = FECache
                       .getCallToRetFlowFunction(
                           Problem, AtInstruction, RetSite, Callees /*Vec*/,
                           combineIds(AtInstructionId, RetSiteId))
                       .computeTargets(CSFact);

      auto &JumpFns = JumpFunctions[RetSiteId];

      for (ByConstRef<d_t> Fact : Facts) {
        auto FactId = FactCompressor.getOrInsert(Fact);

        auto EF = [&] {
          if constexpr (ComputeValues) {
            auto CEF = FECache.getCallToRetEdgeFunction(
                Problem, AtInstruction, CSFact, RetSite, Fact, Callees /*Vec*/,
                combineIds(AtInstructionId, RetSiteId),
                combineIds(PropagatedFactId, FactId));
            return Problem.extend(SourceEF, std::move(CEF));
          } else {
            return EdgeFunctionPtrType{};
          }
        }();

        storeResultsAndPropagate(JumpFns, RetSiteId, SourceFactId, FactId,
                                 FunId, std::move(EF));
      }
    }

    /// NOTE: A CallSite can never be an exit-inst
  }

  template <typename CalleesTy>
  void handleOrDeferCallFlow(ByConstRef<n_t> AtInstruction,
                             uint32_t AtInstructionId, uint32_t SourceFactId,
                             uint32_t PropagatedFactId,
                             EdgeFunctionPtrType SourceEF,
                             const CalleesTy &Callees, uint32_t FunId) {
    auto CSFact = FactCompressor[PropagatedFactId];
    for (ByConstRef<f_t> Callee : Callees) {
      auto CalleeId = FunCompressor.getOrInsert(Callee);
      auto SummaryFF =
          FECache.getSummaryFlowFunction(Problem, AtInstruction, Callee,
                                         combineIds(AtInstructionId, CalleeId));

      if (SummaryFF == nullptr) {
        /// No summary. Start inTRA propagation for the callee and defer
        /// return-propagation
        deferCallFlow(AtInstruction, AtInstructionId, SourceFactId, CSFact,
                      PropagatedFactId, SourceEF, Callee, CalleeId, FunId);
      } else {
        /// Apply SummaryFF and ignore this CSCallee pair in the
        /// inter-propagation
        applySummaryFlow(SummaryFF.computeTargets(CSFact), AtInstruction,
                         AtInstructionId, SourceFactId, CSFact,
                         PropagatedFactId, SourceEF, FunId);
      }
    }
  }

  void countSummaryLinearSearch(size_t SearchLen, size_t NumSummaries) {
    if constexpr (UseEndSummaryTab) {
      SearchLen = NumSummaries;
    }

    if constexpr (EnableStatistics) {
      this->TotalNumLinearSearchForSummary++;
      this->CumulLinearSearchLenForSummary += SearchLen;
      this->MaxLenLinearSearchForSummary =
          std::max(this->MaxLenLinearSearchForSummary, SearchLen);
      this->CumulDiffNumSummariesFound += (SearchLen - NumSummaries);
      this->MaxDiffNumSummariesFound =
          std::max(this->MaxDiffNumSummariesFound, (SearchLen - NumSummaries));

      this->CumulRelDiffNumSummariesFound +=
          SearchLen ? double(NumSummaries) / double(SearchLen) : 1;
    }
  }

  void applyEarlySummariesAtCall(ByConstRef<n_t> AtInstruction,
                                 uint32_t AtInstructionId,
                                 ByConstRef<f_t> Callee, uint32_t CalleeId,
                                 uint32_t FactId, uint32_t SourceFactId,
                                 uint32_t FunId, EdgeFunctionPtrType CallEF) {
    /* if (HasResults)*/ {
      /// Lines 15.2-15.6 in Naeem's paper:
      auto CallerId = [this, FunId, AtInstruction] {
        if constexpr (EnableJumpFunctionGC != JumpFunctionGCMode::Disabled) {
          (void)this;
          (void)AtInstruction;
          return FunId;
        } else {
          (void)FunId;
          return FunCompressor.getOrInsert(ICFG.getFunctionOf(AtInstruction));
        }
      }();

      summaries_t Summaries;
      if constexpr (UseEndSummaryTab) {
        const auto &SummariesTab =
            EndSummaryTab.getOrCreate(combineIds(CalleeId, FactId));
        Summaries = summaries_t(SummariesTab.begin(), SummariesTab.end());
      }

      for (ByConstRef<n_t> ExitInst : ICFG.getExitPointsOf(Callee)) {
        auto ExitId = NodeCompressor.getOrInsert(ExitInst);

        if constexpr (!UseEndSummaryTab) {
          Summaries = JumpFunctions[ExitId].allOf(
              [](uint64_t Key) { return splitId(Key).first; }, FactId,
              [](uint64_t Key) { return splitId(Key).second; });
        }

        countSummaryLinearSearch(JumpFunctions[ExitId].size(),
                                 Summaries.size());

        if (!Summaries.empty()) {
          propagateProcedureSummaries(Summaries, AtInstruction, AtInstructionId,
                                      Callee, CalleeId, ExitInst, ExitId,
                                      SourceFactId, CallerId, CallEF);
        }
      }
    }
  }

  void deferCallFlow(ByConstRef<n_t> AtInstruction, uint32_t AtInstructionId,
                     uint32_t SourceFactId, ByConstRef<d_t> CSFact,
                     uint32_t CSFactId, EdgeFunctionPtrType SourceEF,
                     ByConstRef<f_t> Callee, uint32_t CalleeId,
                     uint32_t FunId) {
    auto CalleeFacts =
        FECache
            .getCallFlowFunction(Problem, AtInstruction, Callee,
                                 combineIds(AtInstructionId, CalleeId))
            .computeTargets(CSFact);

    EdgeFunctionPtrType IdEF = [] {
      if constexpr (ComputeValues) {
        return EdgeIdentity<l_t>{};
      } else {
        return EdgeFunctionPtrType{};
      }
    }();
    for (ByConstRef<n_t> SP : ICFG.getStartPointsOf(Callee)) {
      auto SPId = NodeCompressor.getOrInsert(SP);
      auto &JumpFn = JumpFunctions[SPId];

      for (ByConstRef<d_t> Fact : CalleeFacts) {
        auto FactId = FactCompressor.getOrInsert(Fact);

        auto CallEF = [&] {
          if constexpr (ComputeValues) {
            auto CEF = FECache.getCallEdgeFunction(
                Problem, AtInstruction, CSFact, Callee, Fact,
                combineIds(AtInstructionId, CalleeId),
                combineIds(CSFactId, FactId));
            return Problem.extend(SourceEF, std::move(CEF));
          } else {
            return EdgeFunctionPtrType{};
          }
        }();

        storeResultsAndPropagate(JumpFn, SPId, FactId, FactId, CalleeId, IdEF);

        auto It = &AllInterPropagationsOwner.emplace_back(InterPropagationJob{
            CallEF, SourceFactId, CalleeId, AtInstructionId, FactId});
        auto Inserted =
            AllInterPropagations.insert(InterPropagationJobRef{It}).second;
        if (Inserted) {
          applyEarlySummariesAtCall(AtInstruction, AtInstructionId, Callee,
                                    CalleeId, FactId, SourceFactId, FunId,
                                    CallEF);

          {
            /// Reverse lookup
            auto *&InterJob = SourceFactAndFuncToInterJob.getOrCreate(
                combineIds(FactId, CalleeId));
            It->NextWithSameSourceFactAndCallee = InterJob;
            InterJob = &*It;
          }

          if constexpr (ComputeValues) {
            /// Forward lookup
            auto *&InterJob = SourceFactAndCSToInterJob.getOrCreate(
                combineIds(SourceFactId, AtInstructionId));
            It->NextWithSameSourceFactAndCS = InterJob;
            InterJob = &*It;
          }
        } else {
          /// The InterPropagationJob was already there, so we don't need to own
          /// it
          AllInterPropagationsOwner.pop_back();
        }
      }
    }
  }

  template <typename SummaryFactsTy>
  void applySummaryFlow(const SummaryFactsTy &SummaryFacts,
                        ByConstRef<n_t> AtInstruction, uint32_t AtInstructionId,
                        uint32_t SourceFactId, ByConstRef<d_t> CSFact,
                        uint32_t CSFactId, EdgeFunctionPtrType SourceEF,
                        uint32_t FunId) {
    for (ByConstRef<n_t> RetSite : ICFG.getReturnSitesOfCallAt(AtInstruction)) {
      auto RetSiteId = NodeCompressor.getOrInsert(RetSite);
      auto &JumpFns = JumpFunctions[RetSiteId];

      for (ByConstRef<d_t> Fact : SummaryFacts) {
        auto FactId = FactCompressor.getOrInsert(Fact);

        auto EF = [&] {
          if constexpr (ComputeValues) {
            auto EF = FECache.getSummaryEdgeFunction(
                Problem, AtInstruction, CSFact, RetSite, Fact,
                combineIds(AtInstructionId, RetSiteId),
                combineIds(CSFactId, FactId));
            return EF ? Problem.extend(SourceEF, std::move(EF)) : SourceEF;
          } else {
            return EdgeFunctionPtrType{};
          }
        }();

        storeResultsAndPropagate(JumpFns, RetSiteId, SourceFactId, FactId,
                                 FunId, std::move(EF));
      }
    }
  }

  void propagateProcedureSummaries(const summaries_t &Summaries,
                                   ByConstRef<n_t> CallSite, uint32_t CSId,
                                   ByConstRef<f_t> Callee, uint32_t CalleeId,
                                   ByConstRef<n_t> ExitInst, uint32_t ExitId,
                                   uint32_t SourceFact, uint32_t CallerId,
                                   EdgeFunctionPtrType CallEF) {
    for (ByConstRef<n_t> RetSite : ICFG.getReturnSitesOfCallAt(CallSite)) {
      auto RSId = NodeCompressor.getOrInsert(RetSite);
      auto RetFF = FECache.getRetFlowFunction(
          Problem, CallSite, Callee, ExitInst, RetSite,
          combineIds(CSId, ExitId), combineIds(CalleeId, RSId));

      auto &RSJumpFns = JumpFunctions[RSId];

      for (const auto &Summary : Summaries) {
        uint32_t SummaryFactId{Summary.first};
        auto SummaryFact = FactCompressor[SummaryFactId];
        auto RetFacts = RetFF.computeTargets(SummaryFact);
        for (ByConstRef<d_t> RetFact : RetFacts) {
          auto RetFactId = FactCompressor.getOrInsert(RetFact);

          auto EF = [&]() mutable {
            if constexpr (ComputeValues) {
              auto RetEF = FECache.getReturnEdgeFunction(
                  Problem, CallSite, Callee, ExitInst, SummaryFact, RetSite,
                  RetFact, ExitId, combineIds(CSId, RSId),
                  combineIds(SummaryFactId, RetFactId));
              return Problem.extend(Problem.extend(CallEF, Summary.second),
                                    std::move(RetEF));
            } else {
              return EdgeFunctionPtrType{};
            }
          }();

          storeResultsAndPropagate(RSJumpFns, RSId, SourceFact, RetFactId,
                                   CallerId, std::move(EF));
        }
      }
    }
  }

  void processInterJobs() {
    PHASAR_LOG_LEVEL(INFO, "processInterJobs: " << CallWL.size()
                                                << " relevant calls");

    /// Here, no other job is running concurrently, so we save and reset the
    /// CallWL, such that we can start concurrent jobs in the loop below
    std::vector<uint64_t> RelevantCalls(CallWL.begin(), CallWL.end());

    scope_exit FinishedInterCalls = [] {
      PHASAR_LOG_LEVEL(INFO, "> end inter calls");
    };

    if constexpr (EnableStatistics) {
      if (CallWL.size() > this->CallWLHighWatermark) {
        this->CallWLHighWatermark = CallWL.size();
      }
    }

    CallWL.clear();

    for (auto SourceFactAndFunc : RelevantCalls) {
      auto [SPFactId, CalleeId] = splitId(SourceFactAndFunc);
      auto Callee = FunCompressor[CalleeId];

      if constexpr (EnableStatistics) {
        this->TotalNumRelevantCalls++;
      }

      summaries_t Summaries;
      if constexpr (UseEndSummaryTab) {
        const auto &SummariesTab =
            EndSummaryTab.getOrCreate(combineIds(CalleeId, SPFactId));
        Summaries = summaries_t(SummariesTab.begin(), SummariesTab.end());
      }

      size_t NumInterJobs = 0;
      for (ByConstRef<n_t> ExitInst : ICFG.getExitPointsOf(Callee)) {
        auto ExitId = NodeCompressor.getOrInsert(ExitInst);

        /// Copy the JumpFns, because in the below loop we are calling
        /// getOrCreate on JumpFunctions again and in a tight recursion
        /// ExitId and RSId might be the same and inserting into the same
        /// map we are iterating over is bad

        if constexpr (!UseEndSummaryTab) {
          Summaries = JumpFunctions[ExitId].allOf(
              [](uint64_t Key) { return splitId(Key).first; }, SPFactId,
              [](uint64_t Key) { return splitId(Key).second; });
        }

        countSummaryLinearSearch(JumpFunctions[ExitId].size(),
                                 Summaries.size());

        for (const InterPropagationJob *InterJob =
                 SourceFactAndFuncToInterJob.getOr(SourceFactAndFunc, nullptr);
             InterJob; InterJob = InterJob->NextWithSameSourceFactAndCallee) {

          auto CSId = InterJob->CallSite;
          auto CallSite = NodeCompressor[CSId];
          auto CallerId =
              FunCompressor.getOrInsert(ICFG.getFunctionOf(CallSite));

          if constexpr (EnableStatistics) {
            ++NumInterJobs;
          }
          propagateProcedureSummaries(
              Summaries, CallSite, CSId, Callee, CalleeId, ExitInst, ExitId,
              InterJob->SourceFact, CallerId, InterJob->SourceEF);
        }
      }

      if constexpr (EnableStatistics) {
        this->CumulNumInterJobsPerRelevantCall += NumInterJobs;
        this->MaxNumInterJobsPerRelevantCall =
            std::max(this->MaxNumInterJobsPerRelevantCall, NumInterJobs);
      }
    }
  }

  void submitInitialValues()
    requires ComputeValues
  {
    auto Seeds = Problem.initialSeeds();
    for (const auto &[Inst, SeedMap] : Seeds.getSeeds()) {
      auto InstId = NodeCompressor.getOrInsert(Inst);
      for (const auto &[Fact, Val] : SeedMap) {
        auto FactId = FactCompressor.getOrInsert(Fact);

        WLProp.emplace(ValuePropagationJob{InstId, FactId, Val});
      }
    }
  }

  void propagateValue(uint32_t SPId, uint32_t FactId, l_t Val)
    requires ComputeValues
  {

    /// TODO: Unbalanced return-sites

    auto SP = NodeCompressor[SPId];

    PHASAR_LOG_LEVEL_CAT(DEBUG, "IterativeIDESolver",
                         "propagateValue for N: "
                             << NToString(SP)
                             << "; D: " << DToString(FactCompressor[FactId])
                             << "; L: " << LToString(Val));

    {
      ByConstRef<l_t> StoredVal = get(SPId, FactId);
      auto NewVal = Problem.join(StoredVal, Val);
      if (NewVal == StoredVal) {
        /// Nothing new, so we have already seen this ValuePropagationJob
        /// before.
        /// NOTE: We propagate Bottom for the ZeroFact, such that at the
        /// first iteration we always get a change here
        /// NOTE: Need this early exit for termination in case of recursion
        PHASAR_LOG_LEVEL_CAT(DEBUG, "IterativeIDESolver",
                             "> Value has already been seen!");

        return;
      }
      set(SPId, FactId, std::move(NewVal));
    }

    PHASAR_LOG_LEVEL_CAT(DEBUG, "IterativeIDESolver",
                         "> Make ValuePropagationJob");

    WLComp.insert(SPId);

    auto Fun = ICFG.getFunctionOf(SP);

    for (ByConstRef<n_t> CS : ICFG.getCallsFromWithin(Fun)) {
      PHASAR_LOG_LEVEL_CAT(DEBUG, "IterativeIDESolver",
                           "> CS: " << NToString(CS));

      auto InstId = NodeCompressor.getOrInsert(CS);

      const InterPropagationJob *InterJobs =
          SourceFactAndCSToInterJob.getOr(combineIds(FactId, InstId), nullptr);

      for (; InterJobs; InterJobs = InterJobs->NextWithSameSourceFactAndCS) {
        auto Callee = FunCompressor[InterJobs->Callee];

        PHASAR_LOG_LEVEL_CAT(DEBUG, "IterativeIDESolver",
                             ">> Callee: " << FToString(Callee));

        for (ByConstRef<n_t> CalleeSP : ICFG.getStartPointsOf(Callee)) {
          auto CalleeSPId = NodeCompressor.getOrInsert(CalleeSP);

          PHASAR_LOG_LEVEL_CAT(
              DEBUG, "IterativeIDESolver",
              "> emplace { N: "
                  << NToString(CalleeSP) << "; D: "
                  << DToString(FactCompressor[InterJobs->FactInCallee])
                  << "; L: "
                  << LToString(InterJobs->SourceEF.computeTarget(Val))
                  << " } into WLProp");

          WLProp.emplace(
              ValuePropagationJob{CalleeSPId, InterJobs->FactInCallee,
                                  InterJobs->SourceEF.computeTarget(Val)});

          if constexpr (EnableStatistics) {
            if (WLProp.size() > this->WLPropHighWatermark) {
              this->WLPropHighWatermark = WLProp.size();
            }
          }
        }
      }
    }
  }

  void computeValues(uint32_t SPId)
    requires ComputeValues
  {
    auto SP = NodeCompressor[SPId];
    auto Fun = ICFG.getFunctionOf(SP);

    for (ByConstRef<n_t> Inst : ICFG.getAllInstructionsOf(Fun)) {
      if (Inst == SP) {
        continue;
      }

      if constexpr (EnableJumpFunctionGC ==
                        JumpFunctionGCMode::EnabledAggressively &&
                    has_isInteresting_v<ProblemTy>) {
        if (!Problem.isInteresting(Inst)) {
          continue;
        }
      }

      auto InstId = NodeCompressor.getOrInsert(Inst);
      for (auto [SrcTgtFactId, EF] : JumpFunctions[InstId].cells()) {
        auto [SrcFactId, TgtFactId] = splitId(SrcTgtFactId);

        ByConstRef<l_t> Val = get(SPId, SrcFactId);
        ByConstRef<l_t> StoredVal = get(InstId, TgtFactId);

        auto NewVal = Problem.join(StoredVal, EF.computeTarget(std::move(Val)));
        if (NewVal != StoredVal) {
          set(InstId, TgtFactId, std::move(NewVal));
        }
      }
    }
  }

  llvm::SmallBitVector getCollectableFunctions() {
    llvm::SmallVector<unsigned> FunWorkList;
    FunWorkList.reserve(CandidateFunctionsForGC.count());

    llvm::SmallBitVector FinalCandidates(CandidateFunctionsForGC.size());

    for (auto C : CandidateFunctionsForGC.set_bits()) {
      if (RefCountPerFunction[C]) {
        auto Fun = FunCompressor[C];
        const auto &Callers = ICFG.getCallersOf(Fun);
        for (ByConstRef<n_t> CS : Callers) {
          FunWorkList.push_back(
              FunCompressor.getOrInsert(ICFG.getFunctionOf(CS)));
        }
      } else {
        FinalCandidates.set(C);
      }
    }

    while (!FunWorkList.empty()) {
      auto FunId = FunWorkList.pop_back_val();

      if (!FinalCandidates.test(FunId)) {
        continue;
      }

      FinalCandidates.reset(FunId);

      auto Fun = FunCompressor[FunId];
      const auto &Callers = ICFG.getCallersOf(Fun);
      for (ByConstRef<n_t> CS : Callers) {
        FunWorkList.push_back(
            FunCompressor.getOrInsert(ICFG.getFunctionOf(CS)));
      }
    }

    return FinalCandidates;
  }

  void removeJumpFunctionsFor(ByConstRef<n_t> Inst) {
    if (keepAnalysisInformationAt<false>(Inst)) {
      return;
    }

    size_t InstId = NodeCompressor.getOrInsert(Inst);

    if constexpr (EnableJumpFunctionGC ==
                  JumpFunctionGCMode::EnabledAggressively) {
      static_assert(
          has_isInteresting_v<ProblemTy>,
          "Aggressive JumpFunctionGC requires the TabulationProblem "
          "to define a function 'bool isInteresting(n_t)' that prevents "
          "analysis information at that instruction to be removed. "
          "Otherwise, the analysis results will be empty!");
      // this->base_results_t::ValTab[InstId].clear();
    }

    if constexpr (EnableStatistics) {
      this->NumPathEdges -= JumpFunctions[InstId].size();
    }

    JumpFunctions[InstId].clear();
  }

  void cleanupInterJobsFor(unsigned FunId) {
    auto Cells = SourceFactAndFuncToInterJob.cells();
    for (auto Iter = Cells.begin(), End = Cells.end(); Iter != End;) {
      auto It = Iter++;
      auto CalleeId = splitId(It->first).second;
      if (CalleeId == FunId) {
        SourceFactAndFuncToInterJob.erase(It);
      }
    }
  }

  void collectFunction(unsigned FunId) {
    for (ByConstRef<n_t> Inst :
         ICFG.getAllInstructionsOf(FunCompressor[FunId])) {
      removeJumpFunctionsFor(Inst);
    }

    cleanupInterJobsFor(FunId);

    CandidateFunctionsForGC.reset(FunId);
  }

  void runGC() {
    PHASAR_LOG_LEVEL(INFO, "runGC() with " << CandidateFunctionsForGC.count()
                                           << " candidates");

    size_t NumCollectedFuns = 0;

    scope_exit FinishGC = [&NumCollectedFuns] {
      PHASAR_LOG_LEVEL(INFO, "> Finished GC run (collected " << NumCollectedFuns
                                                             << " functions)");
    };

    auto FinalCandidates = getCollectableFunctions();

    for (auto Candidate : FinalCandidates.set_bits()) {
      collectFunction(Candidate);
      ++NumCollectedFuns;
    }
  }

  static constexpr uint64_t combineIds(uint32_t LHS, uint32_t RHS) noexcept {
    return (uint64_t(LHS) << 32) | RHS;
  }
  static constexpr std::pair<uint32_t, uint32_t> splitId(uint64_t Id) noexcept {
    return {uint32_t(Id >> 32), uint32_t(Id & UINT32_MAX)};
  }

  ProblemTy &Problem;
  const ICFGTy &ICFG;

  Compressor<f_t> FunCompressor{};

  worklist_t<PropagationJob> WorkList{NodeCompressor, ICFG};

  /// --> Begin InterPropagationJobs

  StableVector<InterPropagationJob> AllInterPropagationsOwner;
  /// Intentionally storing pointers, since we capture them in the containers
  /// below
  DenseSet<InterPropagationJobRef, InterPropagationJobRefDSI>
      AllInterPropagations{};
  /// Stores keys of the SourceFactAndFuncToInterJob map. All mapped values
  /// should be part of the set
  DenseSet<uint64_t> CallWL{};
  // Stores pointers into AllInterPropagations building up an intrusive
  // linked list of all jobs matching the key (CalleeSourceFact x Callee).
  // == Reverse Lookup
  DenseTable1d<uint64_t, const InterPropagationJob *>
      SourceFactAndFuncToInterJob{};
  // Stores pointers into AllInterPropagations building up an intrusive
  // linked list of all jobs matching the key (CSSourceFact x CS)
  // == Forward Lookup
  DenseTable1d<uint64_t, const InterPropagationJob *>
      SourceFactAndCSToInterJob{};

  /// <-- End InterPropagationJobs

  worklist_t<ValuePropagationJob> WLProp{NodeCompressor, ICFG};
  DenseSet<uint32_t> WLComp{};

  /// Index represents the instruction
  llvm::SmallVector<SummaryEdges, 0> JumpFunctions{};
  /// Index represents the function
  std::conditional_t<UseEndSummaryTab, map_t<uint64_t, SummaryEdges_JF1>,
                     EmptyType>
      EndSummaryTab;

  llvm::OwningArrayRef<size_t> RefCountPerFunction{};
  llvm::BitVector CandidateFunctionsForGC{};

  flow_edge_function_cache_t FECache{Problem};
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_ITERATIVEIDESOLVER_H
