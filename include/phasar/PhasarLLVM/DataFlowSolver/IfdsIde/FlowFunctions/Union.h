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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_UNION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_UNION_H_

#include <set>
#include <vector>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h>

namespace psr {

template <typename D> class Union : public FlowFunction<D> {
protected:
  const std::vector<FlowFunction<D>> funcs;

public:
  Union(const std::vector<FlowFunction<D>> &funcs) : funcs(funcs) {}
  virtual ~Union() = default;
  std::set<D> computeTargets(const D &source) override {
    std::set<D> result;
    for (const FlowFunction<D> &func : funcs) {
      std::set<D> target = func.computeTarget(source);
      result.insert(target.begin(), target.end());
    }
    return result;
  }
  static FlowFunction<D> setunion(const std::vector<FlowFunction<D>> &funcs) {
    std::vector<FlowFunction<D>> vec;
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

#endif
