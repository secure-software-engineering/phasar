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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_IDENTITY_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_IDENTITY_H_

#include <memory>
#include <set>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"

namespace psr {

template <typename D> class Identity : public FlowFunction<D> {
private:
  Identity() = default;

public:
  virtual ~Identity() = default;
  Identity(const Identity &i) = delete;
  Identity &operator=(const Identity &i) = delete;
  // simply return what the user provides
  std::set<D> computeTargets(D source) override { return {source}; }
  static std::shared_ptr<Identity> getInstance() {
    static std::shared_ptr<Identity> instance =
        std::shared_ptr<Identity>(new Identity);
    return instance;
  }
};

} // namespace psr

#endif
