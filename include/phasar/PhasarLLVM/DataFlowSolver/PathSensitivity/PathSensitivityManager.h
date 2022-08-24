#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H

#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/ExplodedSuperGraph.h"
#include "phasar/Utils/AdjacencyList.h"

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
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using graph_type = AdjacencyList<llvm::SmallVector<n_t, 0>>;

  PathSensitivityManager(
      const ExplodedSuperGraph<AnalysisDomainTy> *ESG) noexcept
      : ESG(assertNotNull(ESG)) {}

  /// TODO: Add PathBuilder
  /// TODO: Add LLVMPathConstraints

  graph_type pathsDagTo(n_t Inst, d_t Fact) const {
    /// TODO: implement
  }

  /// TODO: FlowPathSequence pathsTo(n_t, d_t)

private:
  const ExplodedSuperGraph<AnalysisDomainTy> &ESG;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHSENSITIVITYMANAGER_H
