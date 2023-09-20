/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and Others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_TYPECASTEDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_TYPECASTEDGEFUNCTION_H

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"

#include "llvm/Support/raw_ostream.h"

namespace psr::glca {

struct TypecastEdgeFunction {
  using l_t = IDEGeneralizedLCADomain::l_t;

  unsigned Bits{};
  EdgeValue::Type Dest{};

  [[nodiscard]] l_t computeTarget(ByConstRef<l_t> Source) const;

  static EdgeFunction<l_t> compose(EdgeFunctionRef<TypecastEdgeFunction> This,
                                   const EdgeFunction<l_t> &SecondFunction);

  static EdgeFunction<l_t> join(EdgeFunctionRef<TypecastEdgeFunction> This,
                                const EdgeFunction<l_t> &OtherFunction);
};

[[nodiscard]] bool operator==(ByConstRef<TypecastEdgeFunction> LHS,
                              ByConstRef<TypecastEdgeFunction> RHS) noexcept;
llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              ByConstRef<TypecastEdgeFunction> EF);

} // namespace psr::glca

#endif
