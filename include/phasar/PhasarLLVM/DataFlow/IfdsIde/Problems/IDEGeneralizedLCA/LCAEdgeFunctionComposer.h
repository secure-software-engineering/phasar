/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_LCAEDGEFUNCTIONCOMPOSER_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_LCAEDGEFUNCTIONCOMPOSER_H

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCADomain.h"

namespace psr::glca {

struct LCAEdgeFunctionComposer
    : public EdgeFunctionComposer<IDEGeneralizedLCADomain::l_t> {

  static EdgeFunction<IDEGeneralizedLCADomain::l_t>
  join(EdgeFunctionRef<LCAEdgeFunctionComposer> This,
       const EdgeFunction<IDEGeneralizedLCADomain::l_t> &OtherFunction);
};

} // namespace psr::glca

#endif
