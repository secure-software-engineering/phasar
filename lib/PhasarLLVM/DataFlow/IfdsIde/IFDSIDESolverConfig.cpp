/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/IFDSIDESolverConfig.h"

#include "llvm/Support/raw_ostream.h"

using namespace psr;

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   IFDSIDESolverConfig SC) {
  const auto BoolAlpha = [](bool B) { return B ? "true" : "false"; };
  return OS << "IFDSIDESolverConfig:\n"
            << "\tfollowReturnsPastSeeds: "
            << BoolAlpha(SC.followReturnsPastSeeds()) << '\n'
            << "\tautoAddZero: " << BoolAlpha(SC.autoAddZero()) << '\n'
            << "\tcomputeValues: " << BoolAlpha(SC.computeValues()) << '\n'
            << "\trecordEdges: " << BoolAlpha(SC.recordEdges()) << '\n'
            << "\tcomputePersistedSummaries: "
            << BoolAlpha(SC.computePersistedSummaries()) << '\n'
            << "\temitESG: " << BoolAlpha(SC.emitESG());
}
