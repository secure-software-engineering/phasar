/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <string>

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/Utils/IOFormat.h"

using namespace psr;
using namespace std;

namespace psr {

std::string toString(const IOFormat &D) {
  switch (D) {
  default:
#define IO_FORMAT_TYPES(NAME, CMDFLAG, TYPE)                                   \
  case IOFormat::TYPE:                                                         \
    return NAME;                                                               \
    break;
#include "phasar/PhasarLLVM/Utils/IOFormat.def"
  }
}

IOFormat toIOFormat(const std::string &S) {
  IOFormat Type = llvm::StringSwitch<IOFormat>(S)
#define IO_FORMAT_TYPES(NAME, CMDFLAG, TYPE) .Case(NAME, IOFormat::TYPE)
#include "phasar/PhasarLLVM/Utils/IOFormat.def"
                      .Default(IOFormat::None);
  if (Type == IOFormat::None) {
    Type = llvm::StringSwitch<IOFormat>(S)
#define IO_FORMAT_TYPES(NAME, CMDFLAG, TYPE) .Case(CMDFLAG, IOFormat::TYPE)
#include "phasar/PhasarLLVM/Utils/IOFormat.def"
               .Default(IOFormat::None);
  }
  return Type;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const IOFormat &D) {
  return OS << toString(D);
}
} // namespace psr
