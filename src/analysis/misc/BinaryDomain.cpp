/*
 * BinaryDomain.cpp
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#include "BinaryDomain.hh"

const map<string, BinaryDomain> StringToBinaryDomain = {
    {"BOTTOM", BinaryDomain::BOTTOM}, {"TOP", BinaryDomain::TOP}};

const map<BinaryDomain, string> BinaryDomainToString = {
    {BinaryDomain::BOTTOM, "BOTTOM"}, {BinaryDomain::TOP, "TOP"}};

ostream& operator<<(ostream& os, const BinaryDomain& b) {
  return os << BinaryDomainToString.at(b);
}
