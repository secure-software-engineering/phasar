/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Kill.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_KILL_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_KILL_H_

#include <set>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>

namespace psr {

template <typename D> class Kill : public FlowFunction<D> {
protected:
  D killValue;

public:
  Kill(D killValue) : killValue(killValue) {}
  virtual ~Kill() = default;
  std::set<D> computeTargets(D source) override {
    if (source == killValue)
      return {};
    else
      return {source};
  }
};

} // namespace psr

#endif
