/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_SEMIRING_H
#define PHASAR_UTILS_SEMIRING_H

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"

namespace psr {
template <typename AnalysisDomainTy> class SemiRing {
public:
  using l_t = typename AnalysisDomainTy::l_t;

  virtual ~SemiRing() = default;

  virtual EdgeFunction<l_t> extend(const EdgeFunction<l_t> &L,
                                   const EdgeFunction<l_t> &R) {
    return L.composeWith(R);
  }

  virtual EdgeFunction<l_t> combine(const EdgeFunction<l_t> &L,
                                    const EdgeFunction<l_t> &R) {
    return L.joinWith(R);
  }
};
} // namespace psr

#endif // PHASAR_UTILS_SEMIRING_H
