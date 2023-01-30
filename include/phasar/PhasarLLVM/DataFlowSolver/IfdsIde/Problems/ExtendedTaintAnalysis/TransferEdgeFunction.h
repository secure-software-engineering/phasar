/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_TRANSFEREDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_TRANSFEREDGEFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"
#include "phasar/PhasarLLVM/Utils/ByRef.h"

namespace psr::XTaint {
struct TransferEdgeFunction : EdgeFunctionBase<TransferEdgeFunction> {
  using l_t = EdgeDomain;

  BasicBlockOrdering *BBO{};
  const llvm::Instruction *Load{};
  const llvm::Instruction *To{};

  [[nodiscard]] l_t computeTarget(ByConstRef<l_t> Source) const;

  friend bool operator==(const TransferEdgeFunction &LHS,
                         const TransferEdgeFunction &RHS) noexcept;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const TransferEdgeFunction &TRE);
};
} // namespace psr::XTaint

#endif
