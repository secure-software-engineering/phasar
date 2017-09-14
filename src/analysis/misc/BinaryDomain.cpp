/*
 * BinaryDomain.cpp
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#include "BinaryDomain.hh"

ostream& operator<< (ostream& os, const BinaryDomain& b)
{
	switch (static_cast<underlying_type<BinaryDomain>::type>(b)) {
	case 0:
		return os << "BinaryDomain::BOTTOM";
		break;
	case 1:
		return os << "BinaryDomain::TOP";
		break;
	default:
		return os << "BinaryDomain::error";
		break;
	}
}

