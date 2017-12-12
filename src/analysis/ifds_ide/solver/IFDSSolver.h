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

#ifndef ANALYSIS_IFDS_IDE_SOLVER_IFDSSOLVER_H_
#define ANALYSIS_IFDS_IDE_SOLVER_IFDSSOLVER_H_

#include "../../misc/BinaryDomain.h"
#include "IDESolver.h"
#include <memory>
#include <set>

template <typename N, typename D, typename M, typename I>
class IFDSSolver : public IDESolver<N, D, M, BinaryDomain, I> {
public:
  IFDSSolver(IFDSTabulationProblem<N, D, M, I> &ifdsProblem)
      : IDESolver<N, D, M, BinaryDomain, I>(ifdsProblem) {
    cout << "IFDSSolver::IFDSSolver()" << endl;
    cout << ifdsProblem.NtoString(getNthInstruction(
                ifdsProblem.interproceduralCFG().getMethod("main"), 1))
         << endl;
  }

  virtual ~IFDSSolver() = default;

  set<D> ifdsResultsAt(N stmt) {
    set<D> keyset;
    map<D, BinaryDomain> map = resultsAt(stmt);
    for_each(map.begin(), map.end(),
             [&keyset](D d) { keyset.insert(d.first); });
    return keyset;
  }
};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_IFDSSOLVER_HH_ */
