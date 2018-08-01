/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * FlowFunction.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTION_H_

#include <set>

namespace psr {

template <typename D> class FlowFunction {
public:
  virtual ~FlowFunction() = default;
  virtual std::set<D> computeTargets(D source) = 0;
};

} // namespace psr

#endif
