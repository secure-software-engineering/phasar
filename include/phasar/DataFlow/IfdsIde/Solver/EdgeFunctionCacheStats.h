#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_EDGEFUNCTIONCACHESTATS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_EDGEFUNCTIONCACHESTATS_H

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

struct EdgeFunctionCacheStats {
  size_t NormalAndCtrEFCacheCumulSize = 0;
  size_t NormalAndCtrEFCacheCumulSizeReduced = 0;
  double AvgEFPerEdge = 0;
  size_t MedianEFPerEdge = 0;
  size_t MaxEFPerEdge = 0;
  size_t CallEFCacheCumulSize = 0;
  size_t RetEFCacheCumulSize = 0;
  size_t SummaryEFCacheCumulSize = 0;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const EdgeFunctionCacheStats &S);
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_EDGEFUNCTIONCACHESTATS_H
