/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <ostream>

#include <phasar/PhasarLLVM/IfdsIde/SolverConfiguration.h>

using namespace std;
using namespace psr;

namespace psr {

ostream &operator<<(ostream &os, const SolverConfiguration &sc) {
  return os << "SolverConfiguration:\n"
            << "\tfollowReturnsPastSeeds: " << sc.followReturnsPastSeeds << "\n"
            << "\tautoAddZero: " << sc.autoAddZero << "\n"
            << "\tcomputeValues: " << sc.computeValues << "\n"
            << "\trecordEdges: " << sc.recordEdges << "\n"
            << "\tcomputePersistedSummaries: " << sc.computePersistedSummaries;
}

} // namespace psr
