#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/CGSCCs.h"
#include "phasar/ControlFlow/ICFG.h"
#include "phasar/ControlFlow/SparseCFGProvider.h"
#include "phasar/DataFlow/IfdsIde/IFDSIDESolverConfig.h"
#include "phasar/DataFlow/IfdsIde/IFDSProblem.h"
#include "phasar/DataFlow/IfdsIde/Solver/DFIExplodedSupergraph.h"
#include "phasar/DataFlow/IfdsIde/Solver/DFISolverConfig.h"
#include "phasar/DataFlow/IfdsIde/Solver/DFISolverResults.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolverAPIMixin.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/CsrGraph.h"
#include "phasar/Utils/FunctionId.h"
#include "phasar/Utils/IntervalReachability.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/Lazy.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <optional>
#include <utility>

namespace psr {

/// Solves an IFDS problem by materializing the realizable part of the exploded
/// supergraph and answering reachability with DFS-interval labels, following
/// "DFI: An Interprocedural Value-Flow Analysis Framework that Scales to Large
/// Codebases" by Hsu, Hetzelt, and Franz (CGO'23). See
/// docs/DFISolver-Design.md.
///
/// Computes the same results as the IFDS solvers, while evaluating each flow
/// function only once per data-flow fact and instruction. Additionally, with
/// StaticSolverConfigTy::BuildQueryIndex, getReachability() answers
/// context-sensitive reachability queries between arbitrary
/// (instruction, fact) pairs that hold.
template <IFDSProblem ProblemTy,
          typename StaticSolverConfigTy = DFISolverConfig,
          ICFG ICFGTy = typename ProblemTy::ProblemAnalysisDomain::i_t>
class DFISolver : public IDESolverAPIMixin<
                      DFISolver<ProblemTy, StaticSolverConfigTy, ICFGTy>> {
  friend IDESolverAPIMixin<DFISolver<ProblemTy, StaticSolverConfigTy, ICFGTy>>;

public:
  using domain_t = typename ProblemTy::ProblemAnalysisDomain;
  using d_t = typename domain_t::d_t;
  using n_t = typename domain_t::n_t;
  using f_t = typename domain_t::f_t;
  using i_t = ICFGTy;
  using config_t = StaticSolverConfigTy;

  DFISolver(ProblemTy *Problem, const ICFGTy *ICF,
            StaticSolverConfigTy Config = {})
      : DFISolver(Problem, ICF, Config,
                  getProblemSolverConfig(assertNotNull(Problem))) {}

  /// Uses SolverConfig instead of the problem's solver config. Only
  /// followReturnsPastSeeds is respected; AutoAddZero is taken from
  /// StaticSolverConfigTy.
  DFISolver(ProblemTy *Problem, const ICFGTy *ICF,
            StaticSolverConfigTy /*Config*/, IFDSIDESolverConfig SolverConfig)
      : Problem(&assertNotNull(Problem)), ICF(&assertNotNull(ICF)),
        Results(std::make_unique<detail::DFIResultsData<n_t, d_t>>()),
        SolverConfig(SolverConfig) {}

  [[nodiscard]] DFISolverResults<n_t, d_t> getSolverResults() const noexcept {
    return DFISolverResults<n_t, d_t>(Results.get());
  }

  [[nodiscard]] OwningDFISolverResults<n_t, d_t>
  consumeSolverResults() noexcept {
    return OwningDFISolverResults<n_t, d_t>(std::move(Results));
  }

  /// Only valid after solving
  [[nodiscard]] DFIReachability<n_t, d_t> getReachability() const noexcept
    requires(StaticSolverConfigTy::BuildQueryIndex)
  {
    return DFIReachability<n_t, d_t>(Results.get());
  }

  void dumpResults(llvm::raw_ostream &OS = llvm::outs()) const {
    getSolverResults().dumpResults(OS);
  }

  /// Only valid after solving
  void emitTextReport(llvm::raw_ostream &OS = llvm::outs()) {
    Problem->emitTextReport(getSolverResults(), OS);
  }

private:
  using FlowFunctionPtrType = typename ProblemTy::FlowFunctionPtrType;

  struct CalleeInfo {
    FunctionId Callee{};
    FlowFunctionPtrType FF{};
    /// Empty for summary flow functions
    llvm::SmallVector<n_t, 1> StartPoints;
    bool IsSummaryFF{};
  };

  // --- IDESolverAPIMixin

  void doInitialize() {
    for (const auto &Fun : ICF->getAllFunctions()) {
      Funs.getOrInsert(Fun);
    }
    Graph.emplace(computeCGSCCs(*ICF, Funs),
                  SolverConfig.followReturnsPastSeeds());

    auto ZeroId = Results->Facts.getOrInsert(Problem->getZeroValue());
    assert(ZeroId == dfi::FactId::Zero);
    (void)ZeroId;

    auto Seeds = Problem->initialSeeds();
    for (const auto &[Inst, SeedFacts] : Seeds.getSeeds()) {
      auto Fun = Funs.get(ICF->getFunctionOf(Inst));
      for (const auto &[Fact, Value] : SeedFacts) {
        Graph->addSeed(getOrCreateVertex(Inst, Fact, Fun));
      }
    }
  }

  [[nodiscard]] bool doNext() {
    auto Next = Graph->nextAction();
    if (!Next) {
      return false;
    }

    if (Next->Kind == detail::DFIExplodedSupergraph::ActionKind::Expand) {
      expand(Next->Fun);
    } else {
      auto Fun = Next->Fun;
      Graph->summarize(
          Fun,
          [this](dfi::VertexId CallVtx, dfi::EntryId Entry,
                 dfi::VertexId Exit) { applySummary(CallVtx, Entry, Exit); },
          [this, Fun](dfi::VertexId Exit) { returnUnbalanced(Fun, Exit); });
    }
    return true;
  }

  auto doFinalize() & {
    finalizeResults();
    return getSolverResults();
  }

  auto doFinalize() && {
    finalizeResults();
    return consumeSolverResults();
  }

  // --- Flow functions

  dfi::VertexId getOrCreateVertex(ByConstRef<n_t> Inst, ByConstRef<d_t> Fact,
                                  FunctionId Fun) {
    return Graph->getOrCreateVertex(Results->Insts.getOrInsert(Inst),
                                    Results->Facts.getOrInsert(Fact), Fun,
                                    ICF->isExitInst(Inst));
  }

  /// Creates the vertex reached by an intra-procedural edge to Succ
  dfi::VertexId getOrCreateSuccVertex(ByConstRef<n_t> Succ,
                                      ByConstRef<d_t> Fact, FunctionId Fun) {
    if constexpr (has_advanceToNextUser_v<ICFGTy, d_t>) {
      return getOrCreateVertex(ICF->advanceToNextUser(Succ, Fact), Fact, Fun);
    } else {
      return getOrCreateVertex(Succ, Fact, Fun);
    }
  }

  [[nodiscard]] n_t instOf(dfi::VertexId Vtx) const {
    return Results->Insts[Graph->instOf(Vtx)];
  }
  [[nodiscard]] d_t factOf(dfi::VertexId Vtx) const {
    return Results->Facts[Graph->factOf(Vtx)];
  }

  [[nodiscard]] auto computeTargets(const FlowFunctionPtrType &FF,
                                    ByConstRef<d_t> Fact) {
    auto Ret = (*FF).computeTargets(Fact);
    if constexpr (StaticSolverConfigTy::AutoAddZero) {
      if (Problem->isZeroValue(Fact)) {
        Ret.insert(Problem->getZeroValue());
      }
    }
    return Ret;
  }

  /// Adds the summary edges from the call vertex CallVtx that go through the
  /// same-level path from Entry to the exit vertex Exit
  void applySummary(dfi::VertexId CallVtx, dfi::EntryId Entry,
                    dfi::VertexId Exit) {
    n_t CallSite = instOf(CallVtx);
    n_t ExitInst = instOf(Exit);
    d_t ExitFact = factOf(Exit);
    f_t Callee = Funs[Graph->funOf(Entry)];
    auto Caller = Graph->funOf(CallVtx);

    for (const auto &RetSite : ICF->getReturnSitesOfCallAt(CallSite)) {
      auto FF =
          Problem->getRetFlowFunction(CallSite, Callee, ExitInst, RetSite);
      for (const auto &Fact : computeTargets(FF, ExitFact)) {
        Graph->addSummaryEdge(CallVtx,
                              getOrCreateSuccVertex(RetSite, Fact, Caller));
      }
    }
  }

  /// Returns the exit vertex Exit of Fun, which is reachable without calling
  /// context, to all callers of Fun in the call-graph
  void returnUnbalanced(FunctionId Fun, dfi::VertexId Exit) {
    n_t ExitInst = instOf(Exit);
    d_t ExitFact = factOf(Exit);
    f_t Callee = Funs[Fun];

    bool HasCallers = false;
    for (const auto &CallSite : ICF->getCallersOf(Callee)) {
      HasCallers = true;
      auto Caller = Funs.get(ICF->getFunctionOf(CallSite));
      for (const auto &RetSite : ICF->getReturnSitesOfCallAt(CallSite)) {
        auto FF =
            Problem->getRetFlowFunction(CallSite, Callee, ExitInst, RetSite);
        for (const auto &Fact : computeTargets(FF, ExitFact)) {
          Graph->addUnbalancedReturn(
              Exit, getOrCreateSuccVertex(RetSite, Fact, Caller));
        }
      }
    }

    if constexpr (UnbalancedRetSideEffectProvider<ProblemTy>) {
      if (!HasCallers) {
        Problem->applyUnbalancedRetFlowFunctionSideEffects(Callee, ExitInst,
                                                           ExitFact);
      }
    }
  }

  /// Expands all pending vertices of Fun, batched per instruction, such that
  /// each flow function is constructed once per batch.
  void expand(FunctionId Fun) {
    auto Batch = Graph->takeWorklist(Fun);
    for (const auto *It = Batch.begin(), *End = Batch.end(); It != End;) {
      auto Inst = Graph->instOf(*It);
      const auto *RunEnd = std::find_if(It, End, [this, Inst](auto Vtx) {
        return Graph->instOf(Vtx) != Inst;
      });

      n_t Stmt = Results->Insts[Inst];
      llvm::ArrayRef<dfi::VertexId> Run(It, RunEnd);
      if (ICF->isCallSite(Stmt)) {
        expandCall(Stmt, Run, Fun);
      } else {
        expandNormal(Stmt, Run, Fun);
      }
      It = RunEnd;
    }
  }

  void expandNormal(ByConstRef<n_t> Stmt, llvm::ArrayRef<dfi::VertexId> Run,
                    FunctionId Fun) {
    llvm::SmallVector<std::pair<n_t, FlowFunctionPtrType>, 2> NormalFFs;
    for (const auto &Succ : ICF->getSuccsOf(Stmt)) {
      NormalFFs.emplace_back(Succ, Problem->getNormalFlowFunction(Stmt, Succ));
    }

    for (auto Vtx : Run) {
      d_t Fact = factOf(Vtx);
      Graph->beginEdges(Vtx);
      for (const auto &[Succ, FF] : NormalFFs) {
        for (const auto &Target : computeTargets(FF, Fact)) {
          Graph->addEdge(getOrCreateSuccVertex(Succ, Target, Fun));
        }
      }
      Graph->endEdges(Vtx);
    }
  }

  void expandCall(ByConstRef<n_t> CallSite, llvm::ArrayRef<dfi::VertexId> Run,
                  FunctionId Fun) {
    llvm::SmallVector<f_t> Callees;
    for (const auto &Callee : ICF->getCalleesOfCallAt(CallSite)) {
      Callees.push_back(Callee);
    }

    llvm::SmallVector<n_t, 2> RetSites;
    llvm::SmallVector<FlowFunctionPtrType, 2> CallToRetFFs;
    for (const auto &RetSite : ICF->getReturnSitesOfCallAt(CallSite)) {
      RetSites.push_back(RetSite);
      CallToRetFFs.push_back(
          Problem->getCallToRetFlowFunction(CallSite, RetSite, Callees));
    }

    llvm::SmallVector<CalleeInfo, 1> CalleeInfos;
    for (const auto &Callee : Callees) {
      auto &Info = CalleeInfos.emplace_back();
      Info.Callee = Funs.get(Callee);

      if (auto SummaryFF = Problem->getSummaryFlowFunction(CallSite, Callee)) {
        Info.FF = std::move(SummaryFF);
        Info.IsSummaryFF = true;
        continue;
      }

      Info.FF = Problem->getCallFlowFunction(CallSite, Callee);
      for (const auto &StartPoint : ICF->getStartPointsOf(Callee)) {
        Info.StartPoints.push_back(StartPoint);
      }
    }

    for (auto Vtx : Run) {
      d_t Fact = factOf(Vtx);

      Graph->beginEdges(Vtx);
      for (const auto &[RetSite, FF] : llvm::zip(RetSites, CallToRetFFs)) {
        for (const auto &Target : computeTargets(FF, Fact)) {
          Graph->addEdge(getOrCreateSuccVertex(RetSite, Target, Fun));
        }
      }
      for (const auto &Info : CalleeInfos) {
        if (!Info.IsSummaryFF) {
          continue;
        }
        for (const auto &Target : computeTargets(Info.FF, Fact)) {
          for (const auto &RetSite : RetSites) {
            Graph->addEdge(getOrCreateSuccVertex(RetSite, Target, Fun));
          }
        }
      }
      Graph->endEdges(Vtx);

      for (const auto &Info : CalleeInfos) {
        if (Info.IsSummaryFF) {
          continue;
        }
        for (const auto &CalleeFact : computeTargets(Info.FF, Fact)) {
          for (const auto &StartPoint : Info.StartPoints) {
            auto Entry = Graph->getOrCreateEntry(
                getOrCreateVertex(StartPoint, CalleeFact, Info.Callee));
            auto KnownExits = Graph->addCallEdge(Vtx, Entry);
            for (auto Exit : KnownExits) {
              applySummary(Vtx, Entry, Exit);
            }
          }
        }
      }
    }
  }

  // --- Results

  /// Indexes the reversed supergraph. Its roots are the sinks of the
  /// supergraph; all other vertices follow, as vertices on cycles without a
  /// path to a sink are not reachable from any sink.
  [[nodiscard]] IntervalReachabilityIndex<dfi::VertexId> buildBackwardIndex() {
    auto Reversed = CsrGraph<dfi::VertexId>::reversed(Graph->buildSupergraph());
    auto Roots = std::move(Reversed).roots();
    llvm::append_range(Roots, iota<dfi::VertexId>(Reversed.numVertices()));
    return IntervalReachabilityBuilder<dfi::VertexId>().build(Reversed, Roots);
  }

  void finalizeResults() {
    auto &R = *Results;
    if constexpr (StaticSolverConfigTy::BuildQueryIndex) {
      R.IndexDirection = StaticSolverConfigTy::IndexDirection;
      R.Index.emplace(lazy{[&] {
        if constexpr (StaticSolverConfigTy::IndexDirection ==
                      DFIIndexDirection::Backward) {
          return buildBackwardIndex();
        } else {
          return IntervalReachabilityBuilder<dfi::VertexId>().build(
              Graph->buildSupergraph(), Graph->seeds());
        }
      }});
    }

    R.VtxInst = Graph->takeVertexInsts();
    R.VtxFact = Graph->takeVertexFacts();
    auto NumVertices = R.VtxInst.size();

    R.InstOffsets.assign(R.Insts.size() + 1, 0);
    for (auto Inst : R.VtxInst) {
      ++R.InstOffsets[Inst];
    }
    std::partial_sum(R.InstOffsets.begin(), R.InstOffsets.end(),
                     R.InstOffsets.begin());
    R.VerticesAt.resize_for_overwrite(NumVertices);
    for (auto Vtx : llvm::reverse(iota<dfi::VertexId>(NumVertices))) {
      R.VerticesAt[--R.InstOffsets[R.VtxInst[Vtx]]] = Vtx;
    }
    for (auto Inst : iota<dfi::InstId>(R.Insts.size())) {
      auto Vertices = csrSuccsMut(R.InstOffsets, R.VerticesAt, Inst);
      llvm::sort(Vertices, [&R](dfi::VertexId Lhs, dfi::VertexId Rhs) {
        return R.VtxFact[Lhs] < R.VtxFact[Rhs];
      });
    }

    Graph.reset();
    Funs = FunctionCompressor<f_t>();
  }

  ProblemTy *Problem{};
  const ICFGTy *ICF{};
  std::unique_ptr<detail::DFIResultsData<n_t, d_t>> Results;
  IFDSIDESolverConfig SolverConfig;

  /// Only while solving
  FunctionCompressor<f_t> Funs;
  std::optional<detail::DFIExplodedSupergraph> Graph;
};

template <typename ProblemTy, typename ICFGTy>
DFISolver(ProblemTy *, const ICFGTy *)
    -> DFISolver<ProblemTy, DFISolverConfig, ICFGTy>;
template <typename ProblemTy, typename ICFGTy, typename StaticSolverConfigTy>
DFISolver(ProblemTy *, const ICFGTy *, StaticSolverConfigTy)
    -> DFISolver<ProblemTy, StaticSolverConfigTy, ICFGTy>;
template <typename ProblemTy, typename ICFGTy, typename StaticSolverConfigTy>
DFISolver(ProblemTy *, const ICFGTy *, StaticSolverConfigTy,
          IFDSIDESolverConfig)
    -> DFISolver<ProblemTy, StaticSolverConfigTy, ICFGTy>;

/// Solves the given IFDS problem with the DFISolver and returns the owning
/// results
template <IFDSProblem ProblemTy,
          typename StaticSolverConfigTy = DFISolverConfig>
[[nodiscard]] auto solveDFIProblem(ProblemTy &Problem, const ICFG auto &ICF,
                                   StaticSolverConfigTy Config = {}) {
  return DFISolver(&Problem, &ICF, Config).solve();
}

} // namespace psr
