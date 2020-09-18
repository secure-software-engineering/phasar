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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_SOLVERRESULTS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_SOLVERRESULTS_H_

#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>

#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"
#include "phasar/Utils/Table.h"

namespace psr {

template <typename N, typename D, typename L> class SolverResults {
private:
  Table<N, D, L> &results;
  D zeroValue;

public:
  SolverResults(Table<N, D, L> &res_tab, D zv)
      : results(res_tab), zeroValue(zv) {}

  L resultAt(N stmt, D node) const { return results.get(stmt, node); }

  std::unordered_map<D, L> resultsAt(N stmt, bool stripZero = false) const {
    std::unordered_map<D, L> result = results.row(stmt);
    if (stripZero) {
      for (auto it = result.begin(); it != result.end();) {
        if (it->first == zeroValue) {
          it = result.erase(it);
        } else {
          ++it;
        }
      }
    }
    return result;
  }

  // this function only exists for IFDS problems which use BinaryDomain as their
  // value domain L
  template <typename ValueDomain = L,
            typename = typename std::enable_if_t<
                std::is_same_v<ValueDomain, BinaryDomain>>>
  std::set<D> ifdsResultsAt(N stmt) const {
    std::set<D> KeySet;
    std::unordered_map<D, BinaryDomain> ResultMap = this->resultsAt(stmt);
    for (auto FlowFact : ResultMap) {
      KeySet.insert(FlowFact.first);
    }
    return KeySet;
  }
};

} // namespace psr

#endif
