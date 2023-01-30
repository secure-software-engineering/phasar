/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_LCAEDGEFUNCTIONCOMPOSER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_LCAEDGEFUNCTIONCOMPOSER_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"

namespace psr::glca {

struct LCAEdgeFunctionComposer
    : public EdgeFunctionComposer<IDEGeneralizedLCA::l_t> {

  static EdgeFunction<IDEGeneralizedLCA::l_t>
  join(EdgeFunctionRef<LCAEdgeFunctionComposer> This,
       const EdgeFunction<IDEGeneralizedLCA::l_t> &OtherFunction);
};

} // namespace psr::glca

#endif
