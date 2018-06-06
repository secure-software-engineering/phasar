/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Transfer.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_TRANSFER_H_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_TRANSFER_H_

#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <set>

namespace psr {

template <typename D> class Transfer : public FlowFunction<D> {
private:
  D toValue;
  D fromValue;

public:
  Transfer(D toValue, D fromValue) : toValue(toValue), fromValue(fromValue) {}
  virtual ~Transfer() = default;
  std::set<D> computeTargets(D source) override {
    if (source == fromValue)
      return {source, toValue};
    else if (source == toValue)
      return {};
    else
      return {source};
  }
};
} // namespace psr

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_TRANSFER_HH_ */
