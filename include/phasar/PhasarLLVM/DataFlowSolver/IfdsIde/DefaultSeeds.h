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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_DEFAULTSEEDS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_DEFAULTSEEDS_H_

#include <map>
#include <set>
#include <vector>

namespace psr {

template <typename N, typename D> class DefaultSeeds {
public:
  static std::map<N, std::set<D>> make(std::vector<N> node, D zeroNode) {
    std::map<N, std::set<D>> res;
    for (N n : node) {
      res.insert(n, std::set<D>{zeroNode});
    }
    return res;
  }
};

} // namespace psr

#endif
