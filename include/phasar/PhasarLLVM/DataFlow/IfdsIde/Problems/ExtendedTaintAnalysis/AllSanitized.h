/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ALLSANITIZED_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ALLSANITIZED_H

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"
#include "phasar/Utils/ByRef.h"

namespace psr::XTaint {
struct AllSanitized {
  using l_t = EdgeDomain;

  [[nodiscard]] l_t
  computeTarget([[maybe_unused]] ByConstRef<l_t> Source) const noexcept {
    return Sanitized{};
  }

  [[nodiscard]] bool isConstant() const noexcept { return true; }

  static EdgeFunction<l_t> compose(EdgeFunctionRef<AllSanitized> This,
                                   const EdgeFunction<l_t> &SecondFunction);
  static EdgeFunction<l_t> join(EdgeFunctionRef<AllSanitized> This,
                                const EdgeFunction<l_t> &OtherFunction);
};
} // namespace psr::XTaint

#endif // PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_ALLSANITIZED_H
