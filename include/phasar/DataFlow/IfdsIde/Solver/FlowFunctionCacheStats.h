#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWFUNCTIONCACHESTATS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWFUNCTIONCACHESTATS_H

/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

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
