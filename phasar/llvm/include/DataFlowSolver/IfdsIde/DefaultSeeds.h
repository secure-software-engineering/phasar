/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * DefaultSeeds.h
 *
 *  Created on: 14.10.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_DEFAULTSEEDS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_DEFAULTSEEDS_H

#include <map>
#include <set>
#include <vector>

namespace psr {

template <typename N, typename D> class DefaultSeeds {
public:
  static std::map<N, std::set<D>> make(std::vector<N> Nodes, D ZeroNode) {
    std::map<N, std::set<D>> Result;
    for (N Node : Nodes) {
      Result.insert(std::move(Node), {ZeroNode});
    }
    return Result;
  }
};

} // namespace psr

#endif
