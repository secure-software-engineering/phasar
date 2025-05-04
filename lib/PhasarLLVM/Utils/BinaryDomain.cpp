/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/Domain/BinaryDomain.h"

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

std::string psr::to_string(BinaryDomain B) {
  switch (B) {
  case BinaryDomain::BOTTOM:
    return "BOTTOM";
  case BinaryDomain::TOP:
    return "TOP";
  }
  llvm_unreachable(
      "Both TOP and BOTTOM Should already ba handled in the switch above");
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS, BinaryDomain B) {
  return OS << to_string(B);
}
