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

#include <phasar/PhasarLLVM/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/Utils/BinaryDomain.h>

namespace psr {

template <typename N, typename D, typename M, typename I>
class IFDSSolver : public IDESolver<N, D, M, BinaryDomain, I> {
public:
  IFDSSolver(IFDSTabulationProblem<N, D, M, I> &ifdsProblem)
      : IDESolver<N, D, M, BinaryDomain, I>(ifdsProblem) {
    // std::cout << "IFDSSolver::IFDSSolver()" << std::endl;
    // std::cout << ifdsProblem.NtoString(getNthInstruction(
    // ifdsProblem.interproceduralCFG().getMethod("main"), 1))
    //  << std::endl;
  }

  virtual ~IFDSSolver() = default;

  std::set<D> ifdsResultsAt(N stmt) {
    std::set<D> keyset;
    std::unordered_map<D, BinaryDomain> map = this->resultsAt(stmt);
    for (auto d : map) {
      keyset.insert(d.first);
    }
    return keyset;
  }
};

} // namespace psr

#endif
