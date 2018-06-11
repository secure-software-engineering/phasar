/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Union.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_UNION_H_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_UNION_H_

#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <set>
#include <vector>

using namespace std;
namespace psr {

template <typename D> class Union : public FlowFunction<D> {
private:
  const vector<FlowFunction<D>> funcs;

public:
  Union(const vector<FlowFunction<D>> &funcs) : funcs(funcs) {}
  virtual ~Union() = default;
  set<D> computeTargets(const D &source) override {
    set<D> result;
    for (const FlowFunction<D> &func : funcs) {
      set<D> target = func.computeTarget(source);
      result.insert(target.begin(), target.end());
    }
    return result;
  }
  static FlowFunction<D> setunion(const vector<FlowFunction<D>> &funcs) {
    vector<FlowFunction<D>> vec;
    for (const FlowFunction<D> &func : funcs)
      if (func != Identity<D>::getInstance())
        vec.add(func);
    if (vec.size() == 1)
      return vec[0];
    else if (vec.empty())
      return Identity<D>::getInstance();
    return Union(vec);
  }
};
} // namespace psr

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_UNION_HH_ */
