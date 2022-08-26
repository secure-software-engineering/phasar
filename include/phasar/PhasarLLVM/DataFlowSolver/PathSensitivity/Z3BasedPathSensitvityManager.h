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

#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/PathSensitivityManagerBase.h"
#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/PathSensitivityManagerMixin.h"
#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/Z3BasedPathSensitivityConfig.h"

#include "phasar/Utils/GraphTraits.h"
#include "z3++.h"

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

private:
  z3::expr filterOutUnreachableNodes(graph_type &RevDAG,
                                     const Z3BasedPathSensitivityConfig &Config,
                                     LLVMPathConstraints &LPC) const;
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

  Z3BasedPathSensitivityManager(
      const ExplodedSuperGraph<AnalysisDomainTy> *ESG) noexcept
      : mixin_t(ESG) {}
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_Z3BASEDPATHSENSITIVITYMANAGER_H
