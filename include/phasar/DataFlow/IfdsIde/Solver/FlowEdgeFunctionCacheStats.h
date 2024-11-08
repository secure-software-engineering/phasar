#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_FLOWEDGEFUNCTIONCACHESTATS_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_FLOWEDGEFUNCTIONCACHESTATS_H

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
