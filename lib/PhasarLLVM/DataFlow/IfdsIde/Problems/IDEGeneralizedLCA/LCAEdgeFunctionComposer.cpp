/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/LCAEdgeFunctionComposer.h"

namespace psr::glca {

EdgeFunction<IDEGeneralizedLCADomain::l_t> LCAEdgeFunctionComposer::join(
    EdgeFunctionRef<LCAEdgeFunctionComposer> This,
    const EdgeFunction<IDEGeneralizedLCADomain::l_t> &OtherFunction) {
  if (auto Default = defaultJoinOrNull<l_t, 2>(This, OtherFunction)) {
    return Default;
  }

  return JoinEdgeFunction<l_t, 2>::create(This, OtherFunction);
}

} // namespace psr::glca
