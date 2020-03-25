/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_LAMBDAFLOW_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_LAMBDAFLOW_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"
#include <functional>
#include <set>

namespace psr {

template <typename D> class LambdaFlow : public FlowFunction<D> {
private:
  std::function<std::set<D>(D)> flow;

public:
  LambdaFlow(std::function<std::set<D>(D)> f) : flow(f) {}
  virtual ~LambdaFlow() = default;
  std::set<D> computeTargets(D source) override { return flow(source); }
};

} // namespace psr

#endif
