#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/BitSet.h"
#include "phasar/Utils/StrongTypeDef.h"

#include "llvm/ADT/DenseMap.h"

PHASAR_STRONG_TYPEDEF(psr::monoifds, uint32_t, SourceFactId);

namespace psr::monoifds {

using SourceFactSet = BitSet<SourceFactId, llvm::SmallBitVector>;

/// The local analysis state: TargetFact-->{SourceFact}
///
/// \tparam D The type of (target-) data-flow facts
template <typename D>
struct DataFlowEnvironment : llvm::SmallDenseMap<D, SourceFactSet> {
  using llvm::SmallDenseMap<D, SourceFactSet>::SmallDenseMap;

  // For env-versioning
  uint32_t AnalyzedVersion = 0;
  uint32_t Version = 1;
};

} // namespace psr::monoifds
