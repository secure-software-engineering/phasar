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

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_KILL_H_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_KILL_H_

#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <set>

namespace psr {

template <typename D> class Kill : public FlowFunction<D> {
private:
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

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_KILL_HH_ */
