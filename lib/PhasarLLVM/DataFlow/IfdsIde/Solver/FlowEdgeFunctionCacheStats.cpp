#include "phasar/DataFlow/IfdsIde/Solver/FlowEdgeFunctionCacheStats.h"

#include "llvm/Support/raw_ostream.h"

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   const FlowEdgeFunctionCacheStats &Stats) {
  return OS << static_cast<const FlowFunctionCacheStats &>(Stats)
            << static_cast<const EdgeFunctionCacheStats &>(Stats);
}
