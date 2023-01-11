/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Linus Jungemann and others
 *****************************************************************************/

#include "phasar/Utils/Soundness.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"

std::string psr::toString(Soundness S) {
  switch (S) {
#define SOUNDNESS_FLAG_TYPE(NAME, CMDFLAG, DESC)                               \
  case Soundness::NAME:                                                        \
    return #NAME;                                                              \
    break;
#include "phasar/Utils/Soundness.def"
  case Soundness::Invalid:
    return "Invalid";
  }
}

psr::Soundness psr::toSoundness(llvm::StringRef S) {
  Soundness Type = llvm::StringSwitch<Soundness>(S)
#define SOUNDNESS_FLAG_TYPE(NAME, CMDFLAG, DESC) .Case(#NAME, Soundness::NAME)
#include "phasar/Utils/Soundness.def"
                       .Default(Soundness::Invalid);
  if (Type == Soundness::Invalid) {
    Type = llvm::StringSwitch<Soundness>(S)
#define SOUNDNESS_FLAG_TYPE(NAME, CMDFLAG, DESC) .Case(CMDFLAG, Soundness::NAME)
#include "phasar/Utils/Soundness.def"
               .Default(Soundness::Invalid);
  }
  return Type;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS, Soundness S) {
  return OS << toString(S);
}
