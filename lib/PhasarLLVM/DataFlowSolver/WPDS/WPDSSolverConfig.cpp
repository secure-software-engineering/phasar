/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <ostream>

#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSSolverConfig.h"

using namespace std;
using namespace psr;

namespace psr {

WPDSSolverConfig::WPDSSolverConfig(bool RecordWitnesses,
                                   WPDSSearchDirection SearchDirection,
                                   enum WPDSType Wpdsty)
    : RecordWitnesses(RecordWitnesses), Direction(SearchDirection),
      WPDSType(Wpdsty) {}

ostream &operator<<(ostream &OS, const WPDSSolverConfig &SC) {
  return OS << "WPDSSolverConfig:\n"
            << "\trecordWitnesses: " << SC.RecordWitnesses << "\n"
            << "\tsearchDirection: " << SC.Direction << "\n"
            << "\twpdsty: " << SC.WPDSType << "\n";
}

} // namespace psr
