/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ZeroedFlowFunctions.h
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_ZEROEDFLOWFUNCTIONS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_ZEROEDFLOWFUNCTIONS_H_

#include <memory>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>
#include <set>

namespace psr {

template <typename D> class ZeroedFlowFunction : public FlowFunction<D> {
private:
  std::shared_ptr<FlowFunction<D>> delegate;
  D zerovalue;

public:
  ZeroedFlowFunction(std::shared_ptr<FlowFunction<D>> ff, D zv)
      : delegate(ff), zerovalue(zv) {}
  std::set<D> computeTargets(D source) override {
    if (source == zerovalue) {
      std::set<D> result = delegate->computeTargets(source);
      result.insert(zerovalue);
      return result;
    } else {
      return delegate->computeTargets(source);
    }
  }
};
} // namespace psr

#endif
