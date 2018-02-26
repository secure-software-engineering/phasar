/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Identity.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_IDENTITY_H_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_IDENTITY_H_

#include "../FlowFunction.h"
#include <memory>
#include <set>

using namespace std;

template <typename D> class Identity : public FlowFunction<D> {
private:
  Identity() = default;

public:
  virtual ~Identity() = default;
  Identity(const Identity &i) = delete;
  Identity &operator=(const Identity &i) = delete;
  // simply return what the user provides
  set<D> computeTargets(D source) override { return {source}; }
  static shared_ptr<Identity> v() {
    static shared_ptr<Identity> instance = shared_ptr<Identity>(new Identity);
    return instance;
  }
};

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_IDENTITY_HH_ */
