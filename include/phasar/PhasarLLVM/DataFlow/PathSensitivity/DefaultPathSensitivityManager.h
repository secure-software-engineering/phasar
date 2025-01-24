/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Sriteja Kummita and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PATHSENSITIVITY_DEFAULTPATHSENSITIVITYMANAGER_H
#define PHASAR_PHASARLLVM_PATHSENSITIVITY_DEFAULTPATHSENSITIVITYMANAGER_H

#include "phasar/DataFlow/PathSensitivity/DefaultFlowPath.h"
#include "phasar/DataFlow/PathSensitivity/PathSensitivityManagerBase.h"
#include "phasar/DataFlow/PathSensitivity/PathSensitivityManagerMixin.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/GraphTraits.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <filesystem>
#include <system_error>
#include <type_traits>

namespace llvm {
class Instruction;
} // namespace llvm

namespace psr {
class DefaultPathSensitivityManagerBase
    : public PathSensitivityManagerBase<const llvm::Instruction *> {
public:
  using n_t = const llvm::Instruction *;

  static_assert(is_removable_graph_trait_v<graph_traits_t>,
                "Invalid graph type: Must support edge-removal!");

protected:
  DefaultFlowPathSequence<n_t>
  filterAndFlattenRevDag(graph_type &RevDAG, vertex_t Leaf, n_t FinalInst,
                         const PathSensitivityConfig &Config) const;

  static void deduplicatePaths(DefaultFlowPathSequence<n_t> &Paths);
};

template <typename AnalysisDomainTy,
          typename = std::enable_if_t<std::is_same_v<
              typename AnalysisDomainTy::n_t, const llvm::Instruction *>>>
class DefaultPathSensitivityManager
    : public DefaultPathSensitivityManagerBase,
      public PathSensitivityManagerMixin<
          DefaultPathSensitivityManager<AnalysisDomainTy>, AnalysisDomainTy,
          typename DefaultPathSensitivityManagerBase::graph_type> {
  using base_t = PathSensitivityManagerBase<typename AnalysisDomainTy::n_t>;
  using mixin_t = PathSensitivityManagerMixin<DefaultPathSensitivityManager,
                                              AnalysisDomainTy,
                                              typename base_t::graph_type>;

public:
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using typename PathSensitivityManagerBase<n_t>::graph_type;

  explicit DefaultPathSensitivityManager(
      const ExplodedSuperGraph<AnalysisDomainTy> *ESG,
      PathSensitivityConfig Config = {})
      : mixin_t(ESG), Config(Config) {}

  DefaultFlowPathSequence<n_t> pathsTo(n_t Inst, graph_type Graph) const {

    PHASAR_LOG_LEVEL_CAT(
        DEBUG, "PathSensitivityManager",
        "PathsTo with MaxDAGDepth: " << Config.DAGDepthThreshold);

#ifndef NDEBUG
    {
      std::error_code EC;
      llvm::raw_fd_stream ROS(
          "dag-" +
              std::filesystem::path(psr::getFilePathFromIR(Inst))
                  .filename()
                  .string() +
              "-" + psr::getMetaDataID(Inst) + ".dot",
          EC);
      assert(!EC);
      printGraph(Graph, ROS, "DAG", [](llvm::ArrayRef<n_t> PartialPath) {
        std::string Buf;
        llvm::raw_string_ostream ROS(Buf);
        ROS << "[ ";
        llvm::interleaveComma(PartialPath, ROS, [&ROS](const auto *Inst) {
          ROS << psr::getMetaDataID(Inst);
        });
        ROS << " ]";
        ROS.flush();
        return Buf;
      });

      llvm::errs() << "Paths DAG has " << Graph.Roots.size() << " roots\n";
    }
#endif

    vertex_t Leaf = [&Graph] {
      for (auto Vtx : graph_traits_t::vertices(Graph)) {
        if (graph_traits_t::outDegree(Graph, Vtx) == 0) {
          return Vtx;
        }
      }
      llvm_unreachable("Expect the DAG to have a leaf node!");
    }();

    auto Ret = filterAndFlattenRevDag(Graph, Leaf, Inst, Config);

    deduplicatePaths(Ret);

#ifndef NDEBUG
#ifdef DYNAMIC_LOG
    PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager",
                         "Recorded " << Ret.size() << " valid paths:");

    std::string Str;
    for (const DefaultFlowPath<n_t> &Path : Ret) {
      Str.clear();
      llvm::raw_string_ostream ROS(Str);
      ROS << "> ";
      llvm::interleaveComma(Path.Path, ROS,
                            [&ROS](auto *Inst) { ROS << getMetaDataID(Inst); });
      ROS.flush();
      PHASAR_LOG_LEVEL_CAT(DEBUG, "PathSensitivityManager", Str);
    }

#endif // DYNAMIC_LOG
#endif // NDEBUG

    return Ret;
  }

  DefaultFlowPathSequence<n_t> pathsTo(n_t Inst, d_t Fact) const {

    graph_type Dag = this->pathsDagTo(Inst, std::move(Fact), Config);
    return pathsTo(Inst, std::move(Dag));
  }

  [[nodiscard]] const auto &getConfig() const noexcept { return Config; }

private:
  PathSensitivityConfig Config{};
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_PATHSENSITIVITY_DefaultPathSensitivityMANAGER_H
