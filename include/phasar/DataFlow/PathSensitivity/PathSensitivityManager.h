/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H

#include "phasar/DataFlow/PathSensitivity/ExplodedSuperGraph.h"
#include "phasar/DataFlow/PathSensitivity/PathSensitivityConfig.h"
#include "phasar/DataFlow/PathSensitivity/PathSensitivityManagerBase.h"
#include "phasar/DataFlow/PathSensitivity/PathSensitivityManagerMixin.h"
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

template <typename AnalysisDomainTy>
class PathSensitivityManager
    : public PathSensitivityManagerBase<typename AnalysisDomainTy::n_t>,
      public PathSensitivityManagerMixin<
          PathSensitivityManager<AnalysisDomainTy>, AnalysisDomainTy,
          typename PathSensitivityManagerBase<
              typename AnalysisDomainTy::n_t>::graph_type> {
  using base_t = PathSensitivityManagerBase<typename AnalysisDomainTy::n_t>;
  using mixin_t =
      PathSensitivityManagerMixin<PathSensitivityManager, AnalysisDomainTy,
                                  typename base_t::graph_type>;

public:
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using typename PathSensitivityManagerBase<n_t>::graph_type;

  PathSensitivityManager(
      const ExplodedSuperGraph<AnalysisDomainTy> *ESG) noexcept
      : mixin_t(ESG) {}
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H
