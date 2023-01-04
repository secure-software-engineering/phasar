/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and Others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_TYPECASTEDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_TYPECASTEDGEFUNCTION_H

#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"

namespace psr::glca {

class TypecastEdgeFunction
    : public EdgeFunction<IDEGeneralizedLCA::l_t>,
      public std::enable_shared_from_this<TypecastEdgeFunction> {
  unsigned Bits;
  EdgeValue::Type Dest;
  size_t MaxSize;

public:
  TypecastEdgeFunction(unsigned Bits, EdgeValue::Type Dest, size_t MaxSize)
      : Bits(Bits), Dest(Dest), MaxSize(MaxSize) {}

  IDEGeneralizedLCA::l_t computeTarget(IDEGeneralizedLCA::l_t Source) override;

  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> composeWith(
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> SecondFunction)
      override;

  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
  joinWith(std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> OtherFunction)
      override;

  bool equal_to(std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> Other)
      const override;

  void print(llvm::raw_ostream &OS, bool IsForDebug = false) const override;
};

} // namespace psr::glca

#endif
