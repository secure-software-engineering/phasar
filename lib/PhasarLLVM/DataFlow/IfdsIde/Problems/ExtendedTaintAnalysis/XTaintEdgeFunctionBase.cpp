/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/ComposeEdgeFunction.h"

auto psr::XTaint::makeComposeEF(const EdgeFunction<EdgeDomain> &F,
                                const EdgeFunction<EdgeDomain> &G)
    -> EdgeFunction<EdgeDomain> {
  return ComposeEdgeFunction{F, G};
}
