/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_WPDSPROBLEM_H_
#define PHASAR_PHASARLLVM_WPDS_WPDSPROBLEM_H_

#include <set>
#include <string>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSSolverConfig.h>

namespace psr {

class ProjectIRDB;
template <typename T, typename F> class TypeHierarchy;
template <typename V, typename N> class PointsToInfo;

template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class WPDSProblem : public IDETabulationProblem<N, D, F, T, V, L, I> {
  static_assert(std::is_base_of_v<ICFG<N, F>, I>,
                "I must implement the ICFG interface!");

public:
  WPDSProblem(const ProjectIRDB *IRDB, const TypeHierarchy<T, F> *TH,
              const I *ICF, const PointsToInfo<V, N> *PT,
              std::set<std::string> EntryPoints = {})
      : IDETabulationProblem<N, D, F, T, V, L, I>(IRDB, TH, ICF, PT,
                                                  EntryPoints) {}

  virtual ~WPDSProblem() override = default;

  WPDSSolverConfig SolverConf;

  void setWPDSSolverConfig(WPDSSolverConfig Config) { SolverConf = Config; }

  WPDSSolverConfig getWPDSSolverConfig() const { return SolverConf; }
};

} // namespace psr

#endif
