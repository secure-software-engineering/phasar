/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * BinaryDomain.h
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_MISC_BINARYDOMAIN_H_
#define SRC_ANALYSIS_MISC_BINARYDOMAIN_H_

#include <iostream>
#include <map>
using namespace std;

enum class BinaryDomain { BOTTOM = 0, TOP = 1 };

extern const map<string, BinaryDomain> StringToBinaryDomain;

extern const map<BinaryDomain, string> BinaryDomainToString;

ostream &operator<<(ostream &os, const BinaryDomain &b);

#endif /* SRC_ANALYSIS_MISC_BINARYDOMAIN_HH_ */
