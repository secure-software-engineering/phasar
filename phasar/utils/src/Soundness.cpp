/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Linus Jungemann and others
 *****************************************************************************/

#include <ostream>
#include <string>

#include "llvm/ADT/StringSwitch.h"

#include "phasar/Utils/Soundness.h"

using namespace psr;

namespace psr {

std::string toString(const Soundness &S) {
  switch (S) {
  default:
#define SOUNDNESS_FLAG_TYPE(NAME, TYPE)                                        \
  case Soundness::TYPE:                                                        \
    return NAME;                                                               \
    break;
#include "phasar/Utils/Soundness.def"
  }
}

Soundness toSoundness(const std::string &S) {
  Soundness Type = llvm::StringSwitch<Soundness>(S)
#define SOUNDNESS_FLAG_TYPE(NAME, TYPE) .Case(NAME, Soundness::TYPE)
#include "phasar/Utils/Soundness.def"
                       .Default(Soundness::Invalid);
  return Type;
}

std::ostream &operator<<(std::ostream &OS, const Soundness &S) {
  return OS << toString(S);
}

} // namespace psr
