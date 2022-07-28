/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_WPDSPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_WPDSPROBLEM_H

#include <set>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSSolverConfig.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"

namespace psr {

template <typename T, typename F> class TypeHierarchy;
template <typename V, typename N> class PointsToInfo;

template <typename AnalysisDomainTy>
class WPDSProblem : public IDETabulationProblem<AnalysisDomainTy> {
public:
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using l_t = typename AnalysisDomainTy::l_t;
  using db_t = typename AnalysisDomainTy::db_t;

  static_assert(std::is_base_of_v<ICFG<n_t, f_t>, i_t>,
                "I must implement the ICFG interface!");
  static_assert(std::is_base_of_v<ProjectIRDBBase<db_t>, db_t>,
                "db_t must implement the ProjectIRDBBase interface!");

  WPDSProblem(const db_t *IRDB, const TypeHierarchy<t_t, f_t> *TH,
              const i_t *ICF, PointsToInfo<v_t, n_t> *PT,
              std::set<std::string> EntryPoints = {})
      : IDETabulationProblem<AnalysisDomainTy>(IRDB, TH, ICF, PT, EntryPoints) {
  }

  ~WPDSProblem() override = default;

  WPDSSolverConfig SolverConf;

  void setWPDSSolverConfig(WPDSSolverConfig Config) { SolverConf = Config; }

  [[nodiscard]] WPDSSolverConfig getWPDSSolverConfig() const {
    return SolverConf;
  }
};

} // namespace psr

#endif
