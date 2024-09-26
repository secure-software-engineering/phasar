/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/
#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWEDGEFUNCTIONCACHENG_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWEDGEFUNCTIONCACHENG_H

#include "phasar/DataFlow/IfdsIde/Solver/EdgeFunctionCache.h"
#include "phasar/DataFlow/IfdsIde/Solver/FlowEdgeFunctionCacheStats.h"
#include "phasar/DataFlow/IfdsIde/Solver/FlowFunctionCache.h"

#include "llvm/Support/raw_ostream.h"

namespace psr {

template <typename ProblemTy, bool AutoAddZero>
class FlowEdgeFunctionCacheNG
    : public FlowFunctionCache<ProblemTy, AutoAddZero>,
      public EdgeFunctionCache<ProblemTy> {
public:
  explicit FlowEdgeFunctionCacheNG(ProblemTy & /*Problem*/)
      : FlowFunctionCache<ProblemTy, AutoAddZero>(),
        EdgeFunctionCache<ProblemTy>() {}

  void clearFlowFunctions() {
    this->FlowFunctionCache<ProblemTy, AutoAddZero>::clear();
  }

  void clear() {
    this->FlowFunctionCache<ProblemTy, AutoAddZero>::clear();
    this->EdgeFunctionCache<ProblemTy>::clear();
  }

  void reserve(size_t NumInsts, size_t NumCalls, size_t NumFuns) {
    this->FlowFunctionCache<ProblemTy, AutoAddZero>::reserve(NumInsts, NumCalls,
                                                             NumFuns);
    /// TODO: reserve edge functions as well!
  }

  [[nodiscard]] FlowEdgeFunctionCacheStats getStats() const noexcept {
    return {this->FlowFunctionCache<ProblemTy, AutoAddZero>::getStats(),
            this->EdgeFunctionCache<ProblemTy>::getStats()};
  }

  void dumpStats(llvm::raw_ostream &OS) const { OS << getStats(); }
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWEDGEFUNCTIONCACHENG_H
