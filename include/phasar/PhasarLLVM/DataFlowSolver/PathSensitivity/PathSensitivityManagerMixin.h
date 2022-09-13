/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGERMIXIN_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGERMIXIN_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/SolverResults.h"
#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/ExplodedSuperGraph.h"
#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/PathSensitivityConfig.h"
#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/PathSensitivityManagerBase.h"
#include "phasar/Utils/GraphTraits.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"

#include <type_traits>

namespace psr {
template <typename Derived, typename AnalysisDomainTy, typename GraphType>
class PathSensitivityManagerMixin {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;

  static const ExplodedSuperGraph<AnalysisDomainTy> &
  assertNotNull(const ExplodedSuperGraph<AnalysisDomainTy> *ESG) noexcept {
    assert(ESG != nullptr && "The exploded supergraph passed to the "
                             "pathSensitivityManager must not be nullptr!");
    return *ESG;
  }

protected:
  PathSensitivityManagerMixin(
      const ExplodedSuperGraph<AnalysisDomainTy> *ESG) noexcept
      : ESG(assertNotNull(ESG)) {
    static_assert(std::is_base_of_v<PathSensitivityManagerBase<n_t>, Derived>,
                  "Invalid CTRP instantiation: The Derived type must inherit "
                  "from PathSensitivityManagerBase!");
  }

public:
  template <typename FactsRangeTy, typename ConfigTy>
  [[nodiscard]] GraphType
  pathsDagToAll(n_t Inst, FactsRangeTy FactsRange,
                const PathSensitivityConfigBase<ConfigTy> &Config) const {
    graph_type Dag;

    for (const d_t &Fact : FactsRange) {
      auto Nod = ESG.getNodeOrNull(Inst, std::move(Fact));

      if (!Nod) {
        llvm::report_fatal_error(
            "Invalid Instruction-FlowFact pair. Only use those pairs that are "
            "part of the IDE analysis results!");
      }

      auto Rt = pathsToImpl(Inst, Nod, Dag);
      if (Rt != GraphTraits<GraphType>::Invalid) {
        graph_traits_t::addRoot(Dag, Rt);
      }
    }

#ifndef NDEBUG
    if (!static_cast<const Derived *>(this)->assertIsDAG(Dag)) {
      llvm::report_fatal_error("Invariant violated: DAG has a circle in it!");
    } else {
      PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
                           "The DAG indeed has no circles");
    }
#endif

    if (Config.MinimizeDAG) {
      auto Equiv = minimizeGraph(Dag);
      Dag = static_cast<const Derived *>(this)->reverseDAG(
          std::move(Dag), Equiv, Config.DAGDepthThreshold);
    } else {
      Dag = static_cast<const Derived *>(this)->reverseDAG(
          std::move(Dag), Config.DAGDepthThreshold);
    }

    return Dag;
  }

  template <typename ConfigTy, typename L>
  [[nodiscard]] GraphType
  pathsDagTo(n_t Inst, const SolverResults<n_t, d_t, L> &SR,
             const PathSensitivityConfigBase<ConfigTy> &Config) const {
    auto Res = SR.resultsAt(Inst);
    auto FactsRange = llvm::make_first_range(Res);
    return pathsDagToAll(std::move(Inst), FactsRange, Config);
  }

  template <typename ConfigTy>
  [[nodiscard]] GraphType
  pathsDagTo(n_t Inst, d_t Fact,
             const PathSensitivityConfigBase<ConfigTy> &Config) const {

    return pathsDagToAll(std::move(Inst), llvm::ArrayRef(&Fact, 1), Config);
  }

private:
  using Node = typename ExplodedSuperGraph<AnalysisDomainTy>::Node;
  using graph_type = GraphType;
  using graph_traits_t = GraphTraits<graph_type>;
  using vertex_t = typename graph_traits_t::vertex_t;

  struct PathsToContext {
    llvm::DenseMap<Node *, unsigned> Cache;
    llvm::SetVector<unsigned, llvm::SmallVector<unsigned, 0>> CurrPath;
  };

  bool pathsToImplLAInvoke(vertex_t Ret, Node *Vtx, PathsToContext &Ctx,
                           graph_type &RetDag) const {

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto reachedEnd = [](Node *Vtx) { return !Vtx; };

    do {
      graph_traits_t::node(RetDag, Ret).push_back(Vtx->Source);
      Vtx = Vtx->Predecessor;
    } while (!reachedEnd(Vtx) && Vtx->Neighbors.empty());

    if (reachedEnd(Vtx)) {
      return true;
    }

    if (!Ctx.CurrPath.insert(Ret)) {
      PHASAR_LOG_LEVEL(ERROR, "Node " << Ret << " already on path");
      return false;
    }
    scope_exit PopRet = [&Ctx] { Ctx.CurrPath.pop_back(); };

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto traverseNext = [&Ctx, this, Ret, &RetDag](Node *Nxt) {
      auto Succ = pathsToImplLA(Nxt, Ctx, RetDag);
      if (Succ != graph_traits_t::Invalid && !Ctx.CurrPath.contains(Succ)) {
        graph_traits_t::addEdge(RetDag, Ret, Succ);
      }
    };

    for (auto Nxt : Vtx->Neighbors) {
      traverseNext(Nxt);
    }

    traverseNext(Vtx);

    graph_traits_t::dedupOutEdges(RetDag, Ret);

    return graph_traits_t::outDegree(RetDag, Ret) != 0;
  }

  vertex_t pathsToImplLA(Node *Vtx, PathsToContext &Ctx,
                         graph_type &RetDag) const {
    /// Idea: Treat the graph as firstChild-nextSibling notation and always
    /// traverse with one predecessor lookAhead

    auto [It, Inserted] = Ctx.Cache.try_emplace(Vtx, graph_traits_t::Invalid);
    if (!Inserted) {
      return It->second;
    }

    auto Ret =
        graph_traits_t::addNode(RetDag, typename graph_traits_t::value_type());
    // auto Ret = RetDag.addNode();
    It->second = Ret;

    if (!pathsToImplLAInvoke(Ret, Vtx, Ctx, RetDag)) {
      /// NOTE: Don't erase Vtx from Cache to guarantee termination
      Ctx.Cache[Vtx] = graph_traits_t::Invalid;

      if (Ctx.CurrPath.contains(Ret) || !graph_traits_t::pop(RetDag, Ret)) {
        PHASAR_LOG_LEVEL(WARNING, "Cannot remove invalid path at: " << Ret);
        graph_traits_t::node(RetDag, Ret).clear();
      }

      // if (RetDag.isLast(Ret) && !Ctx.CurrPath.contains(Ret)) {
      //   /// Assume, Ret is not referenced by any other node
      //   RetDag.pop();
      // } else {
      //   PHASAR_LOG_LEVEL(WARNING, << "Cannot remove invalid path at: " <<
      //   Ret); RetDag.PartialPath[Ret].clear();
      // }

      return graph_traits_t::Invalid;
    }
    return Ret;
  }

  vertex_t pathsToImpl(n_t QueryInst, Node *Vtx, graph_type &RetDag) const {
    assert(Vtx->Source != QueryInst);

    auto Ret =
        graph_traits_t::addNode(RetDag, typename graph_traits_t::value_type());
    graph_traits_t::node(RetDag, Ret).push_back(QueryInst);
    // RetDag.PartialPath[Ret].push_back(QueryInst);

    PathsToContext Ctx;

    for (auto *NB : Vtx->Neighbors) {
      auto NBNode = pathsToImplLA(NB, Ctx, RetDag);
      if (NBNode != graph_traits_t::Invalid) {
        graph_traits_t::addEdge(RetDag, Ret, NBNode);
        // Succs.push_back(NBNode);
      }
    }
    auto VtxNode = pathsToImplLA(Vtx, Ctx, RetDag);
    if (VtxNode != graph_traits_t::Invalid) {
      graph_traits_t::addEdge(RetDag, Ret, VtxNode);
      // Succs.push_back(VtxNode);
    }

    graph_traits_t::dedupOutEdges(RetDag, Ret);

    /// Deduplicate the successors relation
    // std::sort(Succs.begin(), Succs.end());
    // Succs.erase(std::unique(Succs.begin(), Succs.end()), Succs.end());

    // RetDag.Successors[Ret] = std::move(Succs);
    return Ret;
  }

  const ExplodedSuperGraph<AnalysisDomainTy> &ESG;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGERMIXIN_H
