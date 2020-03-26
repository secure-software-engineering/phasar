/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <ostream>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSIDESolverConfig.h"

using namespace std;
using namespace psr;

namespace psr {

IFDSIDESolverConfig::IFDSIDESolverConfig(bool followReturnsPastSeeds,
                                         bool autoAddZero, bool computeValues,
                                         bool recordEdges, bool emitESG,
                                         bool computePersistedSummaries)
    : followReturnsPastSeeds(followReturnsPastSeeds), autoAddZero(autoAddZero),
      computeValues(computeValues), recordEdges(recordEdges), emitESG(emitESG),
      computePersistedSummaries(computePersistedSummaries) {}

ostream &operator<<(ostream &os, const IFDSIDESolverConfig &sc) {
  return os << "IFDSIDESolverConfig:\n"
            << "\tfollowReturnsPastSeeds: " << sc.followReturnsPastSeeds << "\n"
            << "\tautoAddZero: " << sc.autoAddZero << "\n"
            << "\tcomputeValues: " << sc.computeValues << "\n"
            << "\trecordEdges: " << sc.recordEdges << "\n"
            << "\tcomputePersistedSummaries: " << sc.computePersistedSummaries;
}

} // namespace psr
