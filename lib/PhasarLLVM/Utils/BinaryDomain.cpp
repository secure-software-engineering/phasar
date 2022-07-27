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

#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"
using namespace psr;
using namespace std;

namespace psr {

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, BinaryDomain B) {
  return OS << (B == BinaryDomain::BOTTOM ? "BOTTOM" : "TOP");
}
} // namespace psr
