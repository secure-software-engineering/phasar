/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_TYPECASTEDGEFUNCTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_TYPECASTEDGEFUNCTION_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"

namespace psr {

class TypecastEdgeFunction
    : public EdgeFunction<IDEGeneralizedLCA::l_t>,
      public std::enable_shared_from_this<TypecastEdgeFunction> {
  unsigned bits;
  EdgeValue::Type dest;
  size_t maxSize;

public:
  TypecastEdgeFunction(unsigned bits, EdgeValue::Type dest, size_t maxSize)
      : bits(bits), dest(dest), maxSize(maxSize) {}

  IDEGeneralizedLCA::l_t computeTarget(IDEGeneralizedLCA::l_t source) override;

  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> composeWith(
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> secondFunction)
      override;

  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
  joinWith(std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> otherFunction)
      override;

  bool equal_to(std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> other)
      const override;

  void print(std::ostream &OS, bool isForDebug = false) const override;
};

} // namespace psr

#endif
