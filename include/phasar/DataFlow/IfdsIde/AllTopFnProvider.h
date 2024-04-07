/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_ALLTOPFNPROVIDER_H
#define PHASAR_DATAFLOW_IFDSIDE_ALLTOPFNPROVIDER_H

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/Utils/JoinLattice.h"

namespace psr {
template <typename AnalysisDomainTy, typename = void> class AllTopFnProvider {
public:
  virtual ~AllTopFnProvider() = default;
  /// Returns an edge function that represents the top element of the analysis.
  virtual EdgeFunction<typename AnalysisDomainTy::l_t>
  allTopFunction() const = 0;
};

template <typename AnalysisDomainTy>
class AllTopFnProvider<
    AnalysisDomainTy,
    std::enable_if_t<HasJoinLatticeTraits<typename AnalysisDomainTy::l_t>>> {
public:
  virtual ~AllTopFnProvider() = default;
  /// Returns an edge function that represents the top element of the analysis.
  virtual EdgeFunction<typename AnalysisDomainTy::l_t> allTopFunction() const {
    return AllTop<typename AnalysisDomainTy::l_t>{};
  }
};
} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_ALLTOPFNPROVIDER_H
