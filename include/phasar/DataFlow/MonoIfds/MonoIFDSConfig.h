#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/MonoIfds/IterationStrategy.h"

namespace psr::monoifds {
/// Dynamic configuration for the MonoIFDSSolver
struct MonoIfdsConfig {
  /// Iteration strategy for intra-procedural propagations
  IterationStrategy IterStrategy = IterationStrategy::DedupFIFOQueue;
  /// Dataflow-Environment versioning. Detects and skips redundant propagations
  /// at runtime
  bool EnableEnvVersioning = false;
  /// Whether to re-schedule the analysis of recursive call-sites whenever the
  /// callee's summary changes (true), or defering re-scheduling of call-sites
  /// until all callee-local paths have been analyzed (false).
  bool EagerReturnPropagation = false;
};

} // namespace psr::monoifds
