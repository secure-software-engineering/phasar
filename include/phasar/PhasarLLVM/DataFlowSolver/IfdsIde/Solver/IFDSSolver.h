/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSSolver.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_IFDSSOLVER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_IFDSSOLVER_H_

#include <memory>
#include <set>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSToIDETabulationProblem.h"
#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"

namespace psr {

template <typename OriginalAnalysisDomain> struct AnalysisDomainExtender;

template <typename AnalysisDomainTy>
class IFDSSolver : public IDESolver<AnalysisDomainExtender<AnalysisDomainTy>> {
public:
  using ProblemTy = IFDSTabulationProblem<AnalysisDomainTy>;
  using D = typename AnalysisDomainTy::d_t;
  using N = typename AnalysisDomainTy::n_t;

  IFDSSolver(IFDSTabulationProblem<AnalysisDomainTy> &ifdsProblem)
      : IDESolver<AnalysisDomainExtender<AnalysisDomainTy>>(ifdsProblem) {}

  ~IFDSSolver() override = default;

  std::set<D> ifdsResultsAt(N stmt) {
    std::set<D> KeySet;
    std::unordered_map<D, BinaryDomain> ResultMap = this->resultsAt(stmt);
    for (auto FlowFact : ResultMap) {
      KeySet.insert(FlowFact.first);
    }
    return KeySet;
  }
};

template <typename Problem>
IFDSSolver(Problem &) -> IFDSSolver<typename Problem::ProblemAnalysisDomain>;

template <typename Problem>
using IFDSSolver_P = IFDSSolver<typename Problem::ProblemAnalysisDomain>;

} // namespace psr

#endif
