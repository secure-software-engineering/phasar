#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H

#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/ExplodedSuperGraph.h"
#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/PathSensitivityConfig.h"
#include "phasar/Utils/AdjacencyList.h"
#include "phasar/Utils/DFAMinimizer.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/ErrorHandling.h"

#include <cassert>
#include <type_traits>

namespace psr {
template <typename AnalysisDomainTy> class PathSensitivityManager {

  static ExplodedSuperGraph<AnalysisDomainTy> &
  assertNotNull(ExplodedSuperGraph<AnalysisDomainTy> *ESG) noexcept {
    assert(ESG != nullptr && "The exploded supergraph passed to the "
                             "pathSensitivityManager must not be nullptr!");
    return *ESG;
  }

public:
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using graph_type = AdjacencyList<llvm::SmallVector<n_t, 0>>;

  static_assert(std::is_integral_v<typename GraphTraits<graph_type>::vertex_t>);

  PathSensitivityManager(
      const ExplodedSuperGraph<AnalysisDomainTy> *ESG) noexcept
      : ESG(assertNotNull(ESG)) {}

  /// TODO: Add PathBuilder
  /// TODO: Add LLVMPathConstraints

  graph_type pathsDagTo(n_t Inst, d_t Fact,
                        const PathSensitivityConfig &Config = {}) const {
    auto Nod = ESG.getNodeOrNull(Inst, std::move(Fact));

    if (!Nod) {
      llvm::report_fatal_error(
          "Invalid Instruction-FlowFact pair. Only use those pairs that are "
          "part of the IDE analysis results!");
    }

    graph_type Dag;
    auto Rt = pathsToImpl(Inst, Nod, Dag);
    graph_traits_t::addRoot(Dag, Rt);

#ifndef NDEBUG
    if (!assertIsDAG(Dag)) {
      llvm::report_fatal_error("Invariant violated: DAG has a circle in it!");
    } else {
      PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
                           "The DAG indeed has no circles");
    }
#endif

    if (Config.MinimizeDAG) {
      auto Equiv = minimizeGraph(Dag);
      Dag = reverseDAG(std::move(Dag), Equiv);
    } else {
      Dag = reverseGraph(std::move(Dag));
    }

    /// TODO: assertIsDAG + minimizeDAG + reverseDAG

    return Dag;
  }

  /// TODO: FlowPathSequence pathsTo(n_t, d_t)

private:
  using Node = typename ExplodedSuperGraph<AnalysisDomainTy>::Node;
  using graph_traits_t = GraphTraits<graph_type>;
  using vertex_t = typename graph_traits_t::vertex_t;

  struct PathsToContext {
    llvm::DenseMap<Node *, unsigned> Cache;
    llvm::SetVector<unsigned, llvm::SmallVector<unsigned, 0>> CurrPath;
  };

  bool pathsToImplLAInvoke(vertex_t Ret, Node *Vtx, PathsToContext &Ctx,
                           graph_type &RetDag, bool AutoSkipZero) {

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto reachedEnd = [this, AutoSkipZero](Node *Vtx) {
      return !Vtx || (AutoSkipZero && Vtx->Value == ESG.getZeroValue());
    };

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
    auto traverseNext = [&Ctx, this, Ret, &RetDag, AutoSkipZero](Node *Nxt) {
      auto Succ = pathsToImplLA(Nxt, Ctx, RetDag, AutoSkipZero);
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

  vertex_t pathsToImplLA(Node *Vtx, PathsToContext &Ctx, graph_type &RetDag,
                         bool AutoSkipZero) {
    /// Idea: Treat the graph as firstChild-nextSibling notation and always
    /// traverse with one predecessor lookAhead

    auto [It, Inserted] = Ctx.Cache.try_emplace(Vtx, graph_traits_t::Invalid);
    if (!Inserted) {
      return It->second;
    }

    auto Ret = graph_traits_t::addNode(RetDag, graph_traits_t::value_type());
    // auto Ret = RetDag.addNode();
    It->second = Ret;

    if (!pathsToImplLAInvoke(Ret, Vtx, Ctx, RetDag, AutoSkipZero)) {
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

  vertex_t pathsToImpl(n_t QueryInst, Node *Vtx, graph_type &RetDag,
                       bool AutoSkipZero) {
    assert(Vtx->Source != QueryInst);

    auto Ret = graph_traits_t::addNode(RetDag, graph_traits_t::value_type());
    graph_traits_t::node(RetDag, Ret).push_back(QueryInst);
    // RetDag.PartialPath[Ret].push_back(QueryInst);

    PathsToContext Ctx;

    for (auto *NB : Vtx->Neighbors) {
      auto NBNode = pathsToImplLA(NB, Ctx, RetDag, AutoSkipZero);
      if (NBNode != graph_traits_t::Invalid) {
        graph_traits_t::addEdge(RetDag, Ret, NBNode);
        // Succs.push_back(NBNode);
      }
    }
    auto VtxNode = pathsToImplLA(Vtx, Ctx, RetDag, AutoSkipZero);
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

  bool assertIsDAG(const graph_type &Dag) {
    llvm::BitVector Visited(Dag.size());
    llvm::DenseSet<vertex_t> CurrPath;
    CurrPath.reserve(graph_traits_t::size(Dag));

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto doAssertIsDAG = [&CurrPath, &Visited, &Dag](auto &doAssertIsDAG,
                                                     vertex_t Vtx) {
      if (!CurrPath.insert(Vtx).second) {
        PHASAR_LOG_LEVEL(ERROR, "DAG has circle: Vtx: " << uintptr_t(Vtx));
        return false;
      }

      scope_exit CurrPathPop = [&CurrPath, Vtx] { CurrPath.erase(Vtx); };
      if (Visited.test(Vtx)) {
        /// We have already analyzed this node
        /// NOTE: We must check this _after_ doing the circle check. Otherwise,
        /// that can never be true
        return true;
      }

      Visited.set(Vtx);

      for (auto Succ : graph_traits_t::outEdges(Dag, Vtx)) {
        if (!doAssertIsDAG(doAssertIsDAG, Succ)) {
          return false;
        }
      }
      return true;
    };

    return doAssertIsDAG(doAssertIsDAG, Dag.Root);
  }

  graph_type reverseDAG(graph_type &&Dag, const llvm::IntEqClasses &Equiv,
                        size_t MaxDepth = SIZE_MAX) {

    struct ReverseDAGContext {
      llvm::SmallVector<vertex_t> Cache;
      size_t CurrDepth = 0;
      size_t MaxDepth = 0;
    } Ctx;

    Ctx.Cache.resize(Equiv.getNumClasses(), graph_traits_t::Invalid);
    Ctx.MaxDepth = MaxDepth;

    graph_type Ret{};
    // Ret.Dag = &Dag;
    // Ret.Leaf = Equiv[Dag.Root];
    if constexpr (is_reservable_graph_trait_v<graph_traits_t>) {
      graph_traits_t::reserve(Ret, Equiv.getNumClasses());
    }
    // Ret.Adj.reserve(Equiv.getNumClasses());
    // Ret.Rev2Vtx.reserve(Equiv.getNumClasses());

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto buildReverseDag = [&Ctx, &Ret, &Equiv, &Dag](auto &buildReverseDag,
                                                      vertex_t Vtx) {
      auto Eq = Equiv[Vtx];
      if (Ctx.Cache[Eq] != graph_traits_t::Invalid) {
        return Ctx.Cache[Eq];
      }

      // typename ReverseDAG::vertex_t Rev = Ret.size();
      // Ret.Rev2Vtx.push_back(Vtx);
      // Ret.Adj.emplace_back();
      auto Rev = graph_traits_t::addNode(
          Ret, std::move(graph_traits_t::node(Dag, Vtx)));
      Ctx.Cache[Eq] = Rev;

      if (Ctx.CurrDepth >= Ctx.MaxDepth) {
        graph_traits_t::addRoot(Ret, Rev);
        // Ret.Roots.push_back(Rev);
        return Rev;
      }

      ++Ctx.CurrDepth;
      scope_exit DecreaseDepth = [&Ctx] { --Ctx.CurrDepth; };

      for (auto Succ : graph_traits_t::outEdges(Dag, Vtx)) {
        /// NOTE: Depending on the depth of SuccRev, we can still get DAGs
        /// deeper than MaxDepth!
        /// However, this is not considered harmful as of now - the DAG still
        /// does not exceed a particular program-slice which size is fixed
        auto SuccRev =
            buildReverseDag(buildReverseDag, graph_traits_t::target(Succ));
        graph_traits_t::addEdge(Ret, SuccRev,
                                graph_traits_t::withEdgeTarget(Succ, Rev));
        // Ret.Adj[SuccRev].push_back(Rev);
      }

      if (graph_traits_t::outDegree(Dag, Vtx) == 0) {
        graph_traits_t::addRoot(Ret, Rev);
        // Ret.Roots.push_back(Rev);
      }

      return Rev;
    };

    buildReverseDag(buildReverseDag, Dag.Root);

    return Ret;
  }

  const ExplodedSuperGraph<AnalysisDomainTy> &ESG;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H
