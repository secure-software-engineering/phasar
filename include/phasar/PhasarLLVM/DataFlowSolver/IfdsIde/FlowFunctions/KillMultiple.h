/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_KILLMULTIPLE_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_KILLMULTIPLE_H_

#include <set>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>

namespace psr {

template <typename D> class KillMultiple : public FlowFunction<D> {
protected:
  std::set<D> killValues;

public:
  KillMultiple(std::set<D> killValues) : killValues(killValues) {}
  virtual ~KillMultiple() = default;
  std::set<D> computeTargets(D source) override {
    if (killValues.find(source) != killValues.end())
      return {};
    else
      return {source};
  }
};

} // namespace psr

#endif
