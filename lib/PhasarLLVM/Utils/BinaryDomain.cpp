/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * BinaryDomain.cpp
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#include <ostream>

#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"
using namespace psr;
using namespace std;

namespace psr {

const map<string, BinaryDomain> StringToBinaryDomain = {
    {"BOTTOM", BinaryDomain::BOTTOM}, {"TOP", BinaryDomain::TOP}};

const map<BinaryDomain, string> BinaryDomainToString = {
    {BinaryDomain::BOTTOM, "BOTTOM"}, {BinaryDomain::TOP, "TOP"}};

ostream &operator<<(ostream &os, const BinaryDomain &b) {
  return os << BinaryDomainToString.at(b);
}
} // namespace psr
