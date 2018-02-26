/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Compose.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_COMPOSE_H_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_COMPOSE_H_

#include "../FlowFunction.h"
#include "Identity.h"
#include <set>
#include <vector>

using namespace std;

template <typename D> class Compose : FlowFunction<D> {
private:
  const vector<FlowFunction<D>> funcs;

public:
  Compose(const vector<FlowFunction<D>> &funcs) : funcs(funcs) {}

  virtual ~Compose() = default;

  set<D> computeTargets(const D &source) override {
    set<D> current(source);
    for (const FlowFunction<D> &func : funcs) {
      set<D> next;
      for (const D &d : current) {
        set<D> target = func.computeTargets(d);
        next.insert(target.begin(), target.end());
      }
      current = next;
    }
    return current;
  }

  static shared_ptr<FlowFunction<D>>
  compose(const vector<FlowFunction<D>> &funcs) {
    vector<FlowFunction<D>> vec;
    for (const FlowFunction<D> &func : funcs)
      if (func != Identity<D>::v())
        vec.insert(func);
    if (vec.size == 1)
      return vec[0];
    else if (vec.empty())
      return Identity<D>::v();
    return make_shared<Compose>(vec);
  }
};

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_COMPOSE_HH_ */
