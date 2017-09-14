/*
 * BinaryDomain.hh
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_MISC_BINARYDOMAIN_HH_
#define SRC_ANALYSIS_MISC_BINARYDOMAIN_HH_

#include <iostream>
using namespace std;

enum class BinaryDomain {
	BOTTOM = 0,
	TOP = 1
};

ostream& operator<< (ostream& os, const BinaryDomain& b);

#endif /* SRC_ANALYSIS_MISC_BINARYDOMAIN_HH_ */
