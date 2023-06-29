/******************************************************************************
 * Copyright (c) 2023 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_RESULTWRAPPER_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_RESULTWRAPPER_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMSolverResults.h"

namespace psr {
template <typename n_t, typename d_t, typename l_t>
l_t resultAt(const SolverResults<n_t, d_t, l_t> &Results,
             const llvm::Instruction *Stmt, const llvm::Value *RequestedVal,
             bool InLLVMSSA) {
  const auto &ResultsAtStmt =
      InLLVMSSA ? Results.resultsAtInLLVMSSA(Stmt) : Results.resultsAt(Stmt);
  l_t Result{};
  for (const auto &[ResultFact, ResultVal] : ResultsAtStmt) {
    if (factMatchesLLVMValue(ResultFact, RequestedVal)) {
      Result = JoinLatticeTraits<l_t>::join(ResultVal, Result);
    }
  }
  return Result;
}
} // namespace psr

#endif /* RESULTWRAPPER_H */
