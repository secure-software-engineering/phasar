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

#include "phasar/DataFlow/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"

namespace psr::glca {

class LCAEdgeFunctionComposer
    : public EdgeFunctionComposer<IDEGeneralizedLCA::l_t> {
  size_t MaxSize;

public:
  LCAEdgeFunctionComposer(
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> F,
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> G, size_t MaxSize);
  // IDEGeneralizedLCA::l_t
  // computeTarget(IDEGeneralizedLCA::l_t Source) override;
  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> composeWith(
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> SecondFunction)
      override;

  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
  joinWith(std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> OtherFunction)
      override;
  const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &getFirst() const;
  const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &
  getSecond() const;
};

} // namespace psr::glca

#endif
