#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H

#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/ExplodedSuperGraph.h"

#include <cassert>

namespace psr {
template <typename AnalysisDomainTy> class PathSensitivityManager {

  static ExplodedSuperGraph<AnalysisDomainTy> &
  assertNotNull(ExplodedSuperGraph<AnalysisDomainTy> *ESG) noexcept {
    assert(ESG != nullptr && "The exploded supergraph passed to the "
                             "pathSensitivityManager must not be nullptr!");
    return *ESG;
  }

public:
  PathSensitivityManager(
      const ExplodedSuperGraph<AnalysisDomainTy> *ESG) noexcept
      : ESG(assertNotNull(ESG)) {}

  /// TODO: Add graph_type
  /// TODO: Add PathBuilder
  /// TODO: Add LLVMPathConstraints

  /// TODO: graph_type pathsDagTo(n_t, d_t)

  /// TODO: FlowPathSequence pathsTo(n_t, d_t)

private:
  const ExplodedSuperGraph<AnalysisDomainTy> &ESG;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H
