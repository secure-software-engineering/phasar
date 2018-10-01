/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/WPDS/WPDSOptions.h>

using namespace std;
using namespace psr;

namespace psr {

ostream &operator<<(ostream &os, const WPDSType &s) {
  return os << WPDSTypeToString.at(s);
}

const map<string, WPDSType> StringToWPDSType{{"WPDS", WPDSType::WPDS},
                                             {"EWPDS", WPDSType::EWPDS},
                                             {"FWPDS", WPDSType::FWPDS},
                                             {"SWPDS", WPDSType::SWPDS}};

const map<WPDSType, string> WPDSTypeToString{{WPDSType::WPDS, "WPDS"},
                                             {WPDSType::EWPDS, "EWPDS"},
                                             {WPDSType::FWPDS, "FWPDS"},
                                             {WPDSType::SWPDS, "SWPDS"}};

ostream &operator<<(ostream &os, const SearchDirection &s) {
  return os << SearchDirectionToString.at(s);
}

const map<string, SearchDirection> StringToSearchDirection{
    {"FORWARD", SearchDirection::FORWARD},
    {"BACKWARD", SearchDirection::BACKWARD}};

const map<SearchDirection, string> SearchDirectionToString{
    {SearchDirection::FORWARD, "FORWARD"},
    {SearchDirection::BACKWARD, "BACKWARD"}};

} // namespace psr
