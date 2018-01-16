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

#ifndef ANALYSIS_IFDS_IDE_FLOWFUNCTION_H_
#define ANALYSIS_IFDS_IDE_FLOWFUNCTION_H_

#include <set>

using namespace std;

template <typename D> class FlowFunction {
public:
  virtual ~FlowFunction() = default;
  virtual set<D> computeTargets(D source) = 0;
};

#endif /* ANALYSIS_IFDS_IDE_FLOWFUNCTION_HH_ */
