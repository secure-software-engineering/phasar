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

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"

namespace psr {

template <typename N, typename D, typename F, typename T, typename V,
          typename I>
class IFDSSolver : public IDESolver<N, D, F, T, V, BinaryDomain, I> {
public:
  using ProblemTy = IFDSTabulationProblem<N, D, F, T, V, I>;

  IFDSSolver(IFDSTabulationProblem<N, D, F, T, V, I> &ifdsProblem)
      : IDESolver<N, D, F, T, V, BinaryDomain, I>(ifdsProblem) {}

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
IFDSSolver(Problem &)
    ->IFDSSolver<typename Problem::n_t, typename Problem::d_t,
                 typename Problem::f_t, typename Problem::t_t,
                 typename Problem::v_t, typename Problem::i_t>;

template <typename Problem>
using IFDSSolver_P = IFDSSolver<typename Problem::n_t, typename Problem::d_t,
                                typename Problem::f_t, typename Problem::t_t,
                                typename Problem::v_t, typename Problem::i_t>;

} // namespace psr

#endif
