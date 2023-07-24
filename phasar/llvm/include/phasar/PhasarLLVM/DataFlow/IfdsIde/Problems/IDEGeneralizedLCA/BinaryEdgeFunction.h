/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_BINARYEDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_BINARYEDGEFUNCTION_H

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCADomain.h"

namespace psr::glca {

struct BinaryEdgeFunction {
  using l_t = IDEGeneralizedLCADomain::l_t;

  llvm::BinaryOperator::BinaryOps Op{};
  l_t Const{};
  bool LeftConst{};

  l_t computeTarget(ByConstRef<l_t> Source) const;

  static EdgeFunction<l_t> compose(EdgeFunctionRef<BinaryEdgeFunction> This,
                                   const EdgeFunction<l_t> &SecondFunction);

  static EdgeFunction<l_t> join(EdgeFunctionRef<BinaryEdgeFunction> This,
                                const EdgeFunction<l_t> &OtherFunction);

  bool operator==(const BinaryEdgeFunction &Other) const noexcept;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const BinaryEdgeFunction &EF);
};

} // namespace psr::glca

#endif
