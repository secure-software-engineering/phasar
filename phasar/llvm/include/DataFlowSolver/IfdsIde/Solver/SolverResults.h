/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * SolverResults.h
 *
 *  Created on: 19.09.2018
 *      Author: rleer
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_SOLVERRESULTS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_SOLVERRESULTS_H

#include <set>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"
#include "phasar/Utils/Table.h"

namespace psr {

template <typename N, typename D, typename L> class SolverResults {
private:
  Table<N, D, L> &Results;
  D ZV;

public:
  SolverResults(Table<N, D, L> &ResTab, D ZV) : Results(ResTab), ZV(ZV) {}

  L resultAt(N Stmt, D Node) const { return Results.get(Stmt, Node); }

  std::unordered_map<D, L> resultsAt(N Stmt, bool StripZero = false) const {
    std::unordered_map<D, L> Result = Results.row(Stmt);
    if (StripZero) {
      for (auto It = Result.begin(); It != Result.end();) {
        if (It->first == ZV) {
          It = Result.erase(It);
        } else {
          ++It;
        }
      }
    }
    return Result;
  }

  // this function only exists for IFDS problems which use BinaryDomain as their
  // value domain L
  template <typename ValueDomain = L,
            typename = typename std::enable_if_t<
                std::is_same_v<ValueDomain, BinaryDomain>>>
  std::set<D> ifdsResultsAt(N Stmt) const {
    std::set<D> KeySet;
    std::unordered_map<D, BinaryDomain> ResultMap = this->resultsAt(Stmt);
    for (auto FlowFact : ResultMap) {
      KeySet.insert(FlowFact.first);
    }
    return KeySet;
  }

  [[nodiscard]] std::vector<typename Table<N, D, L>::Cell>
  getAllResultEntries() const {
    return Results.cellVec();
  }
};

} // namespace psr

#endif
