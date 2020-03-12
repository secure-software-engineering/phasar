/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_WPDSOPTIONS_H_
#define PHASAR_PHASARLLVM_WPDS_WPDSOPTIONS_H_

#include <iosfwd>
#include <string>

namespace psr {

enum class WPDSType {
#define WPDS_TYPES(NAME, TYPE) TYPE,
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSType.def"
};

WPDSType to_WPDSType(const std::string &S);

std::string to_string(const WPDSType &T);

std::ostream &operator<<(std::ostream &OS, const WPDSType &T);

enum class WPDSSearchDirection { FORWARD, BACKWARD };

WPDSSearchDirection to_WPDSSearchDirection(const std::string &S);

std::string to_string(const WPDSSearchDirection &S);

std::ostream &operator<<(std::ostream &OS, const WPDSSearchDirection &S);

} // namespace psr

#endif
