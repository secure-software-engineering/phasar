#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel, Eric Bodden.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/MonoIfds/IterationStrategy.h"

namespace psr::monoifds {
struct MonoIfdsConfig {
  IterationStrategy IterStrategy = IterationStrategy::DedupFIFOQueue;
  bool EnableAggressiveLoopPriorization = false;
  bool EnableEnvVersioning = false;
  bool EagerReturnPropagation = false;
};

} // namespace psr::monoifds
