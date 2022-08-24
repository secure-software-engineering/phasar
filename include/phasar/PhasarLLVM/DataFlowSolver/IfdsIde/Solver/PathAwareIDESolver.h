#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/ExplodedSuperGraph.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/Utils/Logger.h"

namespace psr {
template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class PathAwareIDESolver : public IDESolver<AnalysisDomainTy, Container> {
  using base_t = IDESolver<AnalysisDomainTy>;

public:
  using domain_t = AnalysisDomainTy;
  using n_t = typename base_t::n_t;
  using d_t = typename base_t::d_t;
  using container_type = typename base_t::container_type;

  explicit PathAwareIDESolver(
      IDETabulationProblem<domain_t, container_type> &Problem)
      : base_t(Problem), ESG(Problem.getZeroValue(), Problem, Problem) {

    if (Problem.getIFDSIDESolverConfig().autoAddZero()) {
      PHASAR_LOG_LEVEL(
          WARNING,
          "The PathAwareIDESolver is initialized with the option 'autoAddZero' "
          "being set. This might degrade the quality of the computed paths!");
    }
  }

  [[nodiscard]] const ExplodedSuperGraph<domain_t, container_type> &
  getExplicitESG() const noexcept {
    return ESG;
  }

private:
  void saveEdges(n_t Curr, n_t Succ, d_t CurrNode,
                 const container_type &SuccNodes, bool IsInterProc) override {
    ESG.saveEdges(Curr, CurrNode, Succ, SuccNodes, IsInterProc);
  }

  ExplodedSuperGraph<domain_t, container_type> ESG;
};

template <typename ProblemTy>
PathAwareIDESolver(ProblemTy &)
    -> PathAwareIDESolver<typename ProblemTy::ProblemAnalysisDomain>;

} // namespace psr
