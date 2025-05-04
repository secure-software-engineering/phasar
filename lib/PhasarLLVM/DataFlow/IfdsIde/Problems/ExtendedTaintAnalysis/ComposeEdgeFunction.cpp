/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/ComposeEdgeFunction.h"

namespace psr::XTaint {

EdgeFunction<EdgeDomain>
ComposeEdgeFunction::join(EdgeFunctionRef<ComposeEdgeFunction> This,
                          const EdgeFunction<EdgeDomain> &Other) {
  static_assert(IsEdgeFunction<ComposeEdgeFunction>);
  return EdgeFunctionBase<ComposeEdgeFunction>::join(This, Other);
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const ComposeEdgeFunction &CEF) {
  return OS << "COMP[" << &CEF << "| " << CEF.First << " , " << CEF.Second
            << " ]";
}
} // namespace psr::XTaint
