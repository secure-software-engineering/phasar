/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_COMPOSEEDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_COMPOSEEDGEFUNCTION_H

#include "phasar//DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"

#include "llvm/Support/raw_ostream.h"

namespace psr::XTaint {
struct ComposeEdgeFunction : EdgeFunctionComposer<EdgeDomain> {
  using typename EdgeFunctionComposer<EdgeDomain>::l_t;

  static EdgeFunction<EdgeDomain>
  join(EdgeFunctionRef<ComposeEdgeFunction> This,
       const EdgeFunction<EdgeDomain> &Other);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const ComposeEdgeFunction &CEF);
};
} // namespace psr::XTaint

#endif
