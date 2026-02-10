#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_FLOWFUNCTIONCACHESTATS_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_FLOWFUNCTIONCACHESTATS_H

#include <cstddef>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {
struct FlowFunctionCacheStats {
  size_t NormalFFCacheSize{};
  size_t CallFFCacheSize{};
  size_t ReturnFFCacheSize{};
  size_t SimpleRetFFCacheSize{};
  size_t CallToRetFFCacheSize{};
  size_t SummaryFFCacheSize{};

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const FlowFunctionCacheStats &S);
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWFUNCTIONCACHESTATS_H
