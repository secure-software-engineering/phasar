/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_LCAEDGEFUNCTIONCOMPOSER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_LCAEDGEFUNCTIONCOMPOSER_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"

namespace psr {

class LCAEdgeFunctionComposer
    : public EdgeFunctionComposer<IDEGeneralizedLCA::l_t> {
  size_t maxSize;

public:
  LCAEdgeFunctionComposer(
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> F,
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> G, size_t maxSize);
  // IDEGeneralizedLCA::l_t
  // computeTarget(IDEGeneralizedLCA::l_t source) override;
  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> composeWith(
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> secondFunction)
      override;

  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
  joinWith(std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> otherFunction)
      override;
  const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &getFirst() const;
  const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &
  getSecond() const;
};

} // namespace psr

#endif
