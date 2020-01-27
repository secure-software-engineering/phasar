/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Gen.h
 *
 *  Created on: 05.05.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_GENALL_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_GENALL_H_

#include <set>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>

namespace psr {

template <typename D> class GenAll : public FlowFunction<D> {
protected:
  std::set<D> genValues;
  D zeroValue;

public:
  GenAll(std::set<D> genValues, D zeroValue)
      : genValues(genValues), zeroValue(zeroValue) {}
  virtual ~GenAll() = default;
  std::set<D> computeTargets(D source) override {
    if (source == zeroValue) {
      genValues.insert(source);
      return genValues;
    } else {
      return {source};
    }
  }
};

} // namespace psr

#endif
