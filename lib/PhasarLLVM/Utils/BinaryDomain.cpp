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

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS, BinaryDomain B) {
  switch (B) {
  case BinaryDomain::BOTTOM:
    return OS << "BOTTOM";
  case BinaryDomain::TOP:
    return OS << "TOP";
  }
  llvm_unreachable(
      "Both TOP and BOTTOM should already be handled in the swith above!");
}
