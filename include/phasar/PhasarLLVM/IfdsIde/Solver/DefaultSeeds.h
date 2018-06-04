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
 *  Created on: 03.11.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_DEFAULTSEEDS_H_
#define ANALYSIS_IFDS_IDE_SOLVER_DEFAULTSEEDS_H_

#include <llvm/IR/Instruction.h>
#include <map>
#include <set>

using namespace std;
namespace psr {

class DefaultSeeds {
public:
  template <typename N, typename D>
  static map<N, set<D>> make(vector<N> instructions, D zeroNode) {
    map<N, set<D>> res;
    for (const N &n : instructions)
      res.insert({n, set<D>{zeroNode}});
    return res;
  }
};
} // namespace psr

#endif /* ANALYSIS_IFDS_IDE_SOLVER_DEFAULTSEEDS_HH_ */
