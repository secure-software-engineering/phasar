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

#pragma once

#include <set>

namespace psr {

template <typename D> class FlowFunction {
public:
  virtual ~FlowFunction() = default;
  virtual std::set<D> computeTargets(D source) = 0;
};

} // namespace psr
