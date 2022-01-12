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

#ifndef PHASAR_PHASARLLVM_UTILS_BINARYDOMAIN_H_
#define PHASAR_PHASARLLVM_UTILS_BINARYDOMAIN_H_

#include <iosfwd>
#include <map>

namespace psr {

enum class BinaryDomain { BOTTOM = 0, TOP = 1 };

extern const std::map<std::string, BinaryDomain> StringToBinaryDomain;

extern const std::map<BinaryDomain, std::string> BinaryDomainToString;

std::ostream &operator<<(std::ostream &OS, const BinaryDomain &B);

} // namespace psr

#endif
