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

#ifndef ANALYSIS_IFDS_IDE_DEFAULTSEEDS_H_
#define ANALYSIS_IFDS_IDE_DEFAULTSEEDS_H_

#include <map>
#include <set>

using namespace std;

template <typename N, typename D> class DefaultSeeds {
public:
  static map<N, set<D>> make(vector<N> node, D zeroNode) {
    map<N, set<D>> res;
    for (N n : node) {
      res.insert(n, set<D>{zeroNode});
    }
    return res;
  }
};

#endif /* ANALYSIS_IFDS_IDE_DEFAULTSEEDS_HH_ */
