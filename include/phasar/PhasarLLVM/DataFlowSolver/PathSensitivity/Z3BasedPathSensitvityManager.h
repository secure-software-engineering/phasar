/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_Z3BASEDPATHSENSITIVITYMANAGER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_Z3BASEDPATHSENSITIVITYMANAGER_H

#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/FlowPath.h"
#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/PathSensitivityManagerBase.h"
#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/PathSensitivityManagerMixin.h"
#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/Z3BasedPathSensitivityConfig.h"
#include "phasar/Utils/GraphTraits.h"

#include "phasar/Utils/Logger.h"
#include "z3++.h"
#include "llvm/Support/ErrorHandling.h"

#include <type_traits>

namespace llvm {
class Instruction;
} // namespace llvm

namespace psr {
class LLVMPathConstraints;

class Z3BasedPathSensitivityManagerBase
    : public PathSensitivityManagerBase<const llvm::Instruction *> {
public:
  using n_t = const llvm::Instruction *;

  static_assert(is_removable_graph_trait_v<graph_traits_t>,
                "Invalid graph type: Must support edge-removal!");

protected:
  z3::expr filterOutUnreachableNodes(graph_type &RevDAG, vertex_t Leaf,
                                     const Z3BasedPathSensitivityConfig &Config,
                                     LLVMPathConstraints &LPC) const;

  FlowPathSequence<n_t>
  filterAndFlattenRevDag(graph_type &RevDAG, vertex_t Leaf, n_t FinalInst,
                         const Z3BasedPathSensitivityConfig &Config,
                         LLVMPathConstraints &LPC) const;

  void deduplicatePaths(FlowPathSequence<n_t> &Paths) {
    /// Some kind of lexical sort for being able to deduplicate the paths easily
    std::sort(Paths.begin(), Paths.end(),
              [](const FlowPath<n_t> &LHS, const FlowPath<n_t> &RHS) {
                return LHS.size() < RHS.size() ||
                       (LHS.size() == RHS.size() &&
                        std::lexicographical_compare(LHS.begin(), LHS.end(),
                                                     RHS.begin(), RHS.end()));
              });

    Paths.erase(std::unique(Paths.begin(), Paths.end()), Paths.end());
  }
};

template <typename AnalysisDomainTy,
          typename = std::enable_if_t<std::is_same_v<
              typename AnalysisDomainTy::n_t, const llvm::Instruction *>>>
class Z3BasedPathSensitivityManager
    : public Z3BasedPathSensitivityManagerBase,
      public PathSensitivityManagerMixin<
          Z3BasedPathSensitivityManager<AnalysisDomainTy>, AnalysisDomainTy,
          typename Z3BasedPathSensitivityManagerBase::graph_type> {
  using base_t = PathSensitivityManagerBase<typename AnalysisDomainTy::n_t>;
  using mixin_t = PathSensitivityManagerMixin<Z3BasedPathSensitivityManager,
                                              AnalysisDomainTy,
                                              typename base_t::graph_type>;

public:
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using typename PathSensitivityManagerBase<n_t>::graph_type;
  using MaybeFlowPathSeq = std::variant<FlowPathSequence<n_t>, z3::expr>;

  Z3BasedPathSensitivityManager(
      const ExplodedSuperGraph<AnalysisDomainTy> *ESG) noexcept
      : mixin_t(ESG) {}

  FlowPathSequence<n_t> pathsTo(n_t Inst, d_t Fact,
                                const Z3BasedPathSensitivityConfig &Config,
                                LLVMPathConstraints &LPC) const {
    graph_type Dag = this->pathsDagTo(Inst, std::move(Fact));

    vertex_t Leaf = [&Dag] {
      for (auto Vtx : graph_traits_t::vertices(Dag)) {
        if (graph_traits_t::outDegree(Dag, Vtx) == 0) {
          return Vtx;
        }
      }
      llvm_unreachable("Expect the DAG to have a leaf node!");
    }();

    z3::expr Constraint = filterOutUnreachableNodes(Dag, Leaf, Config, LPC);

    if (Constraint.is_false()) {
      PHASAR_LOG_LEVEL_CAT(INFO, "PathSensitivityManager",
                           "The query position is unreachable");
      return FlowPathSequence<n_t>();
    }

    if (graph_traits_t::size(Dag) > Config.DAGSizeThreshold) {
      PHASAR_LOG_LEVEL_CAT(
          INFO, "PathSensitivityManager",
          "Note: The DAG for query @ "
              << getMetaDataID(Inst)
              << " is too large. Don't collect the precise paths "
                 "here");
      return Constraint;
    }

    auto Ret = filterAndFlattenRevDag(Dag, Leaf, Inst, Config, LPC);

    deduplicatePaths(Ret);

#ifndef NDEBUG
#ifdef DYNAMIC_LOG
    PHASAR_LOG_LEVEL_CAT(
        DEBUG, "PathSensitivityManager",
        "Recorded " << Ret.size() << " valid paths:";
        std::string Str; for (const FlowPath<n_t> &Path
                              : Ret) {
          Str.clear();
          llvm::raw_string_ostream ROS(Str);
          ROS << "> ";
          llvm::interleaveComma(Path.Path, ROS, [&ROS](auto *Inst) {
            ROS << getMetaDataID(Inst);
          });
          ROS << ": " << Path.Constraint.to_string();
          ROS.flush();
          S << Str;
        })
#endif // DYNAMIC_LOG
#endif // NDEBUG

    return Ret;
  }
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_Z3BASEDPATHSENSITIVITYMANAGER_H
