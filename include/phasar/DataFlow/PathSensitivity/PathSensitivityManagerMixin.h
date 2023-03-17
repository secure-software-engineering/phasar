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

#include "phasar/DataFlow/IfdsIde/SolverResults.h"
#include "phasar/DataFlow/PathSensitivity/ExplodedSuperGraph.h"
#include "phasar/DataFlow/PathSensitivity/PathSensitivityConfig.h"
#include "phasar/DataFlow/PathSensitivity/PathSensitivityManagerBase.h"
#include "phasar/DataFlow/PathSensitivity/PathTracingFilter.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/DFAMinimizer.h"
#include "phasar/Utils/GraphTraits.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

#include <filesystem>
#include <system_error>
#include <type_traits>

namespace psr {
template <typename Derived, typename AnalysisDomainTy, typename GraphType>
class PathSensitivityManagerMixin {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;

  using NodeRef = typename ExplodedSuperGraph<AnalysisDomainTy>::NodeRef;
  using graph_type = GraphType;
  using graph_traits_t = GraphTraits<graph_type>;
  using vertex_t = typename graph_traits_t::vertex_t;

  struct PathsToContext {
    llvm::DenseMap<size_t, unsigned> Cache;
    llvm::SetVector<unsigned, llvm::SmallVector<unsigned, 0>> CurrPath;
  };

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
  template <
      typename FactsRangeTy, typename ConfigTy,
      typename Filter = DefaultPathTracingFilter,
      typename = std::enable_if_t<is_pathtracingfilter_for_v<Filter, NodeRef>>>
  [[nodiscard]] GraphType
  pathsDagToAll(n_t Inst, FactsRangeTy FactsRange,
                const PathSensitivityConfigBase<ConfigTy> &Config,
                const Filter &PFilter = {}) const {
    graph_type Dag;
    PathsToContext Ctx;

    for (const d_t &Fact : FactsRange) {
      auto Nod = ESG.getNodeOrNull(Inst, std::move(Fact));

      if (!Nod) {

        // llvm::errs() << "At Inst " << llvmIRToString(Inst) << "; Fact: " <<
        // Fact
        //              << '\n';
        // llvm::errs();

        {
          std::error_code EC;
          llvm::raw_fd_ostream ROS("explicitesg-err.dot", EC);
          ESG.printAsDot(ROS);
        }

        llvm::errs()
            << "> ESG written to "
            << std::filesystem::canonical("explicitesg-err.dot").string()
            << '\n';
        llvm::errs().flush();

        llvm::report_fatal_error(
            "Invalid Instruction-FlowFact pair. Only use those pairs that are "
            "part of the IDE analysis results!");
      }

      /// NOTE: We don't need to check that Nod has not been processed yet,
      /// because in the ESG construction we only merge nodes with the same flow
      /// fact. Here, the flow fact for each node differs (assuming FactsRage
      /// does not contain duplicates)

      auto Rt = pathsToImpl(Inst, Nod, Dag, Ctx, PFilter);
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

    // {
    //   std::error_code EC;
    //   llvm::raw_fd_stream ROS(
    //       "revdag-" +
    //           std::filesystem::path(psr::getFilePathFromIR(Inst))
    //               .filename()
    //               .string() +
    //           "-" + psr::getMetaDataID(Inst) + ".dot",
    //       EC);
    //   assert(!EC);
    //   printGraph(Dag, ROS, "DAG", [](llvm::ArrayRef<n_t> PartialPath) {
    //     std::string Buf;
    //     llvm::raw_string_ostream ROS(Buf);
    //     ROS << "[ ";
    //     llvm::interleaveComma(PartialPath, ROS, [&ROS](const auto *Inst) {
    //       ROS << psr::getMetaDataID(Inst);
    //     });
    //     ROS << " ]";
    //     ROS.flush();
    //     return Buf;
    //   });

    //   llvm::errs() << "Rev Paths DAG has " << Dag.Roots.size() << " roots\n";
    // }

#endif

    if (Config.DAGDepthThreshold != SIZE_MAX) {
      Dag = Derived::reverseDAG(std::move(Dag), Config.DAGDepthThreshold);
    } else {
      Dag = reverseGraph(std::move(Dag));
    }

    if (Config.MinimizeDAG) {
      // {
      //   std::error_code EC;
      //   llvm::raw_fd_ostream ROS("unminimized_graph.dot", EC);
      //   assert(!EC);
      //   printGraph(Dag, ROS, "Unminimized DAG", [](const auto &Node) {
      //     std::string Str;
      //     llvm::raw_string_ostream StrStr(Str);
      //     StrStr << "[";
      //     llvm::interleaveComma(Node, StrStr, [&StrStr](const auto *Inst) {
      //       StrStr << getMetaDataID(Inst);
      //     });
      //     StrStr << "]";
      //     return Str;
      //   });
      // }

      auto Equiv = minimizeGraph(Dag);

      // llvm::errs() << "Equiv:\n";
      // llvm::errs() << "> Size: " << Equiv.getNumClasses() << '\n';
      // for (size_t i = 0, end = Equiv.getNumClasses(); i < end; ++i) {
      //   llvm::errs() << "[" << i << "] = " << Equiv[i] << '\n';
      // }

      Dag = createEquivalentGraphFrom(std::move(Dag), Equiv);
      // Dag = Derived::reverseDAG(
      //     std::move(Dag), [&Equiv](vertex_t Vtx) { return Equiv[Vtx]; },
      //     Equiv.getNumClasses(), Config.DAGDepthThreshold);

#ifndef NDEBUG
      if (!static_cast<const Derived *>(this)->assertIsDAG(Dag)) {
        // {
        //   std::error_code EC;
        //   llvm::raw_fd_ostream ROS("minimized_graph.dot", EC);
        //   assert(!EC);
        //   printGraph(Dag, ROS, "Minimized DAG", [](const auto &Node) {
        //     std::string Str;
        //     llvm::raw_string_ostream StrStr(Str);
        //     StrStr << "[";
        //     llvm::interleaveComma(Node, StrStr, [&StrStr](const auto *Inst) {
        //       StrStr << getMetaDataID(Inst);
        //     });
        //     StrStr << "]";
        //     return Str;
        //   });
        // }

        llvm::report_fatal_error("Invariant violated: DAG has a circle in it!");
      } else {
        PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
                             "The DAG indeed has no circles");
      }
#endif
    }

    return Dag;
  }

  template <
      typename ConfigTy, typename L, typename Filter = DefaultPathTracingFilter,
      typename = std::enable_if_t<is_pathtracingfilter_for_v<Filter, NodeRef>>>
  [[nodiscard]] GraphType
  pathsDagTo(n_t Inst, const SolverResults<n_t, d_t, L> &SR,
             const PathSensitivityConfigBase<ConfigTy> &Config,
             const Filter &PFilter = {}) const {
    auto Res = SR.resultsAt(Inst);
    auto FactsRange = llvm::make_first_range(Res);
    return pathsDagToAll(std::move(Inst), FactsRange, Config, PFilter);
  }

  template <
      typename ConfigTy, typename Filter = DefaultPathTracingFilter,
      typename = std::enable_if_t<is_pathtracingfilter_for_v<Filter, NodeRef>>>
  [[nodiscard]] GraphType
  pathsDagTo(n_t Inst, d_t Fact,
             const PathSensitivityConfigBase<ConfigTy> &Config,
             const Filter &PFilter = {}) const {

    return pathsDagToAll(std::move(Inst), llvm::ArrayRef(&Fact, 1), Config,
                         PFilter);
  }

private:
  template <typename Filter>
  bool pathsToImplLAInvoke(vertex_t Ret, NodeRef Vtx, PathsToContext &Ctx,
                           graph_type &RetDag, const Filter &PFilter) const {
    NodeRef Prev;
    bool IsEnd = false;
    bool IsError = false;

    do {
      Prev = Vtx;
      graph_traits_t::node(RetDag, Ret).push_back(Vtx.source());

      if (!Vtx.predecessor()) {
        return true;
      }

      Vtx = Vtx.predecessor();

      if (PFilter.HasReachedEnd(Prev, Vtx)) {
        IsEnd = true;
        break;
      }

      if (PFilter.IsErrorneousTransition(Prev, Vtx)) {
        IsError = true;
        break;
      }

    } while (!Vtx.hasNeighbors());

    if (!Ctx.CurrPath.insert(Ret)) {
      PHASAR_LOG_LEVEL(ERROR, "Node " << Ret << " already on path");
      return false;
    }
    scope_exit PopRet = [&Ctx] { Ctx.CurrPath.pop_back(); };

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto traverseNext = [&Ctx, this, Ret, &RetDag, &PFilter](NodeRef Nxt) {
      auto Succ = pathsToImplLA(Nxt, Ctx, RetDag, PFilter);
      if (Succ != graph_traits_t::Invalid && !Ctx.CurrPath.contains(Succ)) {
        graph_traits_t::addEdge(RetDag, Ret, Succ);
      }
    };

    if (!IsEnd && !IsError) {
      traverseNext(Vtx);
    }

    for (auto Nxt : Vtx.neighbors()) {
      assert(Nxt != nullptr);
      if (PFilter.IsErrorneousTransition(Prev, Nxt)) {
        continue;
      }
      if (PFilter.HasReachedEnd(Prev, Nxt)) {
        IsEnd = true;
        continue;
      }
      traverseNext(Nxt);
    }

    graph_traits_t::dedupOutEdges(RetDag, Ret);

    return IsEnd || graph_traits_t::outDegree(RetDag, Ret) != 0;
  }

  template <typename Filter>
  vertex_t pathsToImplLA(NodeRef Vtx, PathsToContext &Ctx, graph_type &RetDag,
                         const Filter &PFilter) const {
    /// Idea: Treat the graph as firstChild-nextSibling notation and always
    /// traverse with one predecessor lookAhead

    auto [It, Inserted] =
        Ctx.Cache.try_emplace(Vtx.id(), graph_traits_t::Invalid);
    if (!Inserted) {
      return It->second;
    }

    auto Ret =
        graph_traits_t::addNode(RetDag, typename graph_traits_t::value_type());
    // auto Ret = RetDag.addNode();
    It->second = Ret;

    if (!pathsToImplLAInvoke(Ret, Vtx, Ctx, RetDag, PFilter)) {
      /// NOTE: Don't erase Vtx from Cache to guarantee termination
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) -- fp
      Ctx.Cache[Vtx.id()] = graph_traits_t::Invalid;

      if (Ctx.CurrPath.contains(Ret) || !graph_traits_t::pop(RetDag, Ret)) {
        PHASAR_LOG_LEVEL(WARNING, "Cannot remove invalid path at: " << Ret);
        graph_traits_t::node(RetDag, Ret).clear();
      }

      return graph_traits_t::Invalid;
    }
    return Ret;
  }

  template <typename Filter>
  vertex_t pathsToImpl(n_t QueryInst, NodeRef Vtx, graph_type &RetDag,
                       PathsToContext &Ctx, const Filter &PFilter) const {

    auto Ret =
        graph_traits_t::addNode(RetDag, typename graph_traits_t::value_type());
    graph_traits_t::node(RetDag, Ret).push_back(QueryInst);

    for (auto NB : Vtx.neighbors()) {
      auto NBNode = pathsToImplLA(NB, Ctx, RetDag, PFilter);
      if (NBNode != graph_traits_t::Invalid) {
        graph_traits_t::addEdge(RetDag, Ret, NBNode);
      }
    }
    auto VtxNode = pathsToImplLA(Vtx, Ctx, RetDag, PFilter);
    if (VtxNode != graph_traits_t::Invalid) {
      graph_traits_t::addEdge(RetDag, Ret, VtxNode);
    }

    graph_traits_t::dedupOutEdges(RetDag, Ret);

    return Ret;
  }

  const ExplodedSuperGraph<AnalysisDomainTy> &ESG;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGERMIXIN_H
