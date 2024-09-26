#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWEDGEFUNCTIONCACHESTATS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWEDGEFUNCTIONCACHESTATS_H

/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/EdgeFunctionCacheStats.h"
#include "phasar/DataFlow/IfdsIde/Solver/FlowFunctionCacheStats.h"

namespace psr {
struct FlowEdgeFunctionCacheStats : FlowFunctionCacheStats,
                                    EdgeFunctionCacheStats {
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const FlowEdgeFunctionCacheStats &Stats);
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWEDGEFUNCTIONCACHESTATS_H
