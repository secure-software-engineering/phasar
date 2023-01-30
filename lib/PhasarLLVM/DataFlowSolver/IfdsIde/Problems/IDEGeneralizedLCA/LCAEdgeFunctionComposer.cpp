/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/LCAEdgeFunctionComposer.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"

namespace psr::glca {

EdgeFunction<IDEGeneralizedLCA::l_t> LCAEdgeFunctionComposer::join(
    EdgeFunctionRef<LCAEdgeFunctionComposer> This,
    const EdgeFunction<IDEGeneralizedLCA::l_t> &OtherFunction) {
  if (auto Default = defaultJoinOrNull<l_t, 2>(This, OtherFunction)) {
    return Default;
  }

  return JoinEdgeFunction<l_t, 2>::create(This, OtherFunction);
}

} // namespace psr::glca
