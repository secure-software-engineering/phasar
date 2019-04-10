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
#include <map>
#include <string>

namespace psr {

enum class WPDSType { WPDS, EWPDS, FWPDS, SWPDS };

std::ostream &operator<<(std::ostream &os, const WPDSType &s);

extern const std::map<std::string, WPDSType> StringToWPDSType;

extern const std::map<WPDSType, std::string> WPDSTypeToString;

enum class SearchDirection { FORWARD, BACKWARD };

std::ostream &operator<<(std::ostream &os, const SearchDirection &s);

extern const std::map<std::string, SearchDirection> StringToSearchDirection;

extern const std::map<SearchDirection, std::string> SearchDirectionToString;

} // namespace psr

#endif
