/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSSolverConfig.h"

namespace psr {

WPDSSolverConfig::WPDSSolverConfig(bool RecordWitnesses,
                                   WPDSSearchDirection SearchDirection,
                                   enum WPDSType Wpdsty)
    : RecordWitnesses(RecordWitnesses), Direction(SearchDirection),
      WPDSType(Wpdsty) {}

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const WPDSSolverConfig &SC) {
  return OS << "WPDSSolverConfig:\n"
            << "\trecordWitnesses: " << SC.RecordWitnesses << "\n"
            << "\tsearchDirection: " << SC.Direction << "\n"
            << "\twpdsty: " << SC.WPDSType << "\n";
}

} // namespace psr
