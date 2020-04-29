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
    : public EdgeFunctionComposer<IDEGeneralizedLCA::v_t> {
  size_t maxSize;

public:
  LCAEdgeFunctionComposer(
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> F,
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> G, size_t maxSize);
  // IDEGeneralizedLCA::v_t
  // computeTarget(IDEGeneralizedLCA::v_t source) override;
  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> composeWith(
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> secondFunction)
      override;

  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
  joinWith(std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> otherFunction)
      override;
  const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> &getFirst() const;
  const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> &
  getSecond() const;
};

} // namespace psr

#endif
