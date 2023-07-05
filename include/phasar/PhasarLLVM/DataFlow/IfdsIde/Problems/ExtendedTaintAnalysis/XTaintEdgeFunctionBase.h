/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_XTAINTEDGEFUNCTIONBASE_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_XTAINTEDGEFUNCTIONBASE_H

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AllSanitized.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"

#include "llvm/ADT/Hashing.h"

#include <memory>

namespace psr::XTaint {

static constexpr size_t JoinThreshold = 2;

EdgeFunction<EdgeDomain> makeComposeEF(const EdgeFunction<EdgeDomain> &F,
                                       const EdgeFunction<EdgeDomain> &G);

template <typename Derived> struct EdgeFunctionBase {
  static EdgeFunction<EdgeDomain>
  compose(EdgeFunctionRef<Derived> This,
          const EdgeFunction<EdgeDomain> &SecondFunction) {
    if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
      return Default;
    }
    return makeComposeEF(This, SecondFunction);
  }

  static EdgeFunction<EdgeDomain>
  join(EdgeFunctionRef<Derived> This,
       const EdgeFunction<EdgeDomain> &OtherFunction) {
    if (auto Default =
            defaultJoinOrNull<EdgeDomain, JoinThreshold>(This, OtherFunction)) {
      return Default;
    }
    if (llvm::isa<AllSanitized>(OtherFunction)) {
      return This;
    }

    return JoinEdgeFunction<EdgeDomain, JoinThreshold>::create(This,
                                                               OtherFunction);
  }
};

} // namespace psr::XTaint
#endif
