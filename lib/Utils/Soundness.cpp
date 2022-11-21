/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Linus Jungemann and others
 *****************************************************************************/

#include "phasar/Utils/Soundness.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"

using namespace psr;

std::string psr::toString(Soundness S) {
  switch (S) {
  default:
#define SOUNDNESS_FLAG_TYPE(NAME, TYPE)                                        \
  case Soundness::TYPE:                                                        \
    return NAME;                                                               \
    break;
#include "phasar/Utils/Soundness.def"
  }
}

Soundness psr::toSoundness(llvm::StringRef S) {
  Soundness Type = llvm::StringSwitch<Soundness>(S)
#define SOUNDNESS_FLAG_TYPE(NAME, TYPE) .Case(NAME, Soundness::TYPE)
#include "phasar/Utils/Soundness.def"
                       .Default(Soundness::Invalid);
  return Type;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS, Soundness S) {
  return OS << toString(S);
}
