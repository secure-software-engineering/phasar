#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"

#include <cstdint>

namespace psr::monoifds {
/// Iteration strategy for intra-procedural propagation
enum class IterationStrategy : uint8_t {
  /// Simple de-duplicating queue (See ArraySetDriver)
  DedupFIFOQueue,
  /// Reverse-Post-Order queue (see TopoFixpointDriver)
  TopoPrioQueue,
  /// Hybrid of TopoPrioQueue in singleton-CG-SCCs and DedupFIFOQueue for larger
  /// CG-SCCs
  Hybrid,
};

[[nodiscard]] constexpr llvm::StringRef
to_string(IterationStrategy IterStrategy) noexcept {
  switch (IterStrategy) {
  case IterationStrategy::DedupFIFOQueue:
    return "queue";
  case IterationStrategy::TopoPrioQueue:
    return "topo";
  case IterationStrategy::Hybrid:
    return "hybrid";
  }
  llvm_unreachable("All valid IterationStrategy alternatives should be handled "
                   "in the switch above");
}

} // namespace psr::monoifds
