#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel, Eric Bodden.
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
enum class IterationStrategy : uint8_t {
  DedupFIFOQueue,
  TopoPrioQueue,
  Hybrid,
  HybridCapped,
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
  case IterationStrategy::HybridCapped:
    return "hybrid-capped";
  }
  llvm_unreachable("All valid IterationStrategy alternatives should be handled "
                   "in the switch above");
}

} // namespace psr::monoifds
