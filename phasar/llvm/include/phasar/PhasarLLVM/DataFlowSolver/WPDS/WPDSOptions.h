/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_WPDSOPTIONS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_WPDSOPTIONS_H

#include <string>

namespace llvm {
class raw_ostream;
}

namespace psr {

enum class WPDSType {
#define WPDS_TYPES(NAME, TYPE) TYPE,
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSType.def"
};

WPDSType toWPDSType(const std::string &S);

std::string toString(const WPDSType &T);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const WPDSType &T);

enum class WPDSSearchDirection { FORWARD, BACKWARD };

WPDSSearchDirection toWPDSSearchDirection(const std::string &S);

std::string toString(const WPDSSearchDirection &S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const WPDSSearchDirection &S);

} // namespace psr

#endif
