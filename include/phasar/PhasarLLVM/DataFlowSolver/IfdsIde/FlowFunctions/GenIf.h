/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_GENIF_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_GENIF_H_

#include <functional>
#include <set>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"

namespace psr {

/**
 * @brief Generates the given value if the given predicate evaluates to true.
 * @tparam D The type of data-flow facts to be generated.
 */
template <typename D> class GenIf : public FlowFunction<D> {
protected:
  std::set<D> GenValues;
  std::function<bool(D)> Predicate;

public:
  GenIf(D GenValue, std::function<bool(D)> Predicate)
      : GenValues({GenValue}), Predicate(Predicate) {}

  GenIf(std::set<D> GenValues, std::function<bool(D)> Predicate)
      : GenValues(std::move(GenValues)), Predicate(Predicate) {}

  virtual ~GenIf() = default;

  std::set<D> computeTargets(D Source) override {
    if (Predicate(Source)) {
      std::set<D> ToGenerate;
      ToGenerate.insert(Source);
      ToGenerate.insert(GenValues.begin(), GenValues.end());
      return ToGenerate;
    } else {
      return {Source};
    }
  }
};

} // namespace psr

#endif
