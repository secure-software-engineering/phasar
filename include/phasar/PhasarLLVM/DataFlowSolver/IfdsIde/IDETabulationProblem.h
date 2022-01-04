/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IDETabulationProblem.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IDETABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IDETABULATIONPROBLEM_H_

#include <iostream>
#include <memory>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/JoinLattice.h"

namespace psr {

class ProjectIRDB;
template <typename T, typename F> class TypeHierarchy;
template <typename V, typename N> class PointsToInfo;

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IDETabulationProblem
    : public IFDSTabulationProblem<AnalysisDomainTy, Container>,
      public virtual EdgeFunctions<AnalysisDomainTy>,
      public virtual JoinLattice<AnalysisDomainTy>,
      public virtual EdgeFactPrinter<AnalysisDomainTy> {
public:
  using d_t = typename AnalysisDomainTy::d_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using l_t = typename AnalysisDomainTy::l_t;
  using i_t = typename AnalysisDomainTy::i_t;

  static_assert(std::is_base_of_v<ICFG<n_t, f_t>, i_t>,
                "Type parameter i_t must implement the ICFG interface!");

  using typename EdgeFunctions<AnalysisDomainTy>::EdgeFunctionPtrType;

  IDETabulationProblem(const ProjectIRDB *IRDB,
                       const TypeHierarchy<t_t, f_t> *TH, const i_t *ICF,
                       PointsToInfo<v_t, n_t> *PT,
                       std::set<std::string> EntryPoints = {})
      : IFDSTabulationProblem<AnalysisDomainTy, Container>(
            IRDB, TH, ICF, PT, std::move(EntryPoints)) {}

  ~IDETabulationProblem() override = default;

  /// Returns an edge function that represents the top element of the analysis.
  virtual EdgeFunctionPtrType allTopFunction() = 0;
};

} // namespace psr

#endif
