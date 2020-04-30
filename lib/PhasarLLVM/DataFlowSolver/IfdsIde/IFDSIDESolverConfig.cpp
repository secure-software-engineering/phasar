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

IFDSIDESolverConfig::IFDSIDESolverConfig(bool FollowReturnsPastSeeds,
                                         bool AutoAddZero, bool ComputeValues,
                                         bool RecordEdges, bool EmitESG,
                                         bool ComputePersistedSummaries)
    : followReturnsPastSeeds(FollowReturnsPastSeeds), autoAddZero(AutoAddZero),
      computeValues(ComputeValues), recordEdges(RecordEdges), emitESG(EmitESG),
      computePersistedSummaries(ComputePersistedSummaries) {}

ostream &operator<<(ostream &OS, const IFDSIDESolverConfig &SC) {
  return OS << "IFDSIDESolverConfig:\n"
            << "\tfollowReturnsPastSeeds: " << SC.followReturnsPastSeeds << "\n"
            << "\tautoAddZero: " << SC.autoAddZero << "\n"
            << "\tcomputeValues: " << SC.computeValues << "\n"
            << "\trecordEdges: " << SC.recordEdges << "\n"
            << "\tcomputePersistedSummaries: " << SC.computePersistedSummaries;
}

} // namespace psr
