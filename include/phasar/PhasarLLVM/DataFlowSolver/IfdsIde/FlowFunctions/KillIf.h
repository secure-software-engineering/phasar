/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_KILLIF_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_KILLIF_H_

#include <functional>
#include <set>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"

namespace psr {

/**
 * @brief Kills all facts for which the given predicate evaluates to true.
 * @tparam D The type of data-flow facts to be killed.
 */
template <typename D> class KillIf : public FlowFunction<D> {
protected:
  std::function<bool(D)> Predicate;

public:
  KillIf(std::function<bool(D)> Predicate) : Predicate(Predicate) {}
  virtual ~KillIf() = default;
  std::set<D> computeTargets(D source) override {
    if (Predicate(source))
      return {};
    else
      return {source};
  }
};

} // namespace psr

#endif
