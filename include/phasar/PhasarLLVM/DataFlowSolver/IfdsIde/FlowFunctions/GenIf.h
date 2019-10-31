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

#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>

namespace psr {

/**
 * @brief Generates the given value if the given predicate evaluates to true.
 * @tparam D The type of data-flow facts to be generated.
 */
template <typename D> class GenIf : public FlowFunction<D> {
protected:
  D genValue;
  D zeroValue;
  std::function<bool(D)> Predicate;

public:
  GenIf(D genValue, D zeroValue, std::function<bool(D)> Predicate)
      : genValue(genValue), zeroValue(zeroValue), Predicate(Predicate) {}
  virtual ~GenIf() = default;
  std::set<D> computeTargets(D source) override {
    if (Predicate(source))
      return {source, genValue};
    else
      return {source};
  }
};

} // namespace psr

#endif
