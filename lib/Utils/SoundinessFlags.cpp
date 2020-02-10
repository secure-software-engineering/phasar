/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/
#include <phasar/Utils/SoundinessFlags.h>

using namespace psr;

namespace psr {

std::string to_string(const SoundinessFlag &SF) {
  switch (SF) {
  default:
#define SOUNDINESS_FLAG_TYPE(NAME, CMDFLAG, TYPE)                              \
  case SoundinessFlag::TYPE:                                                   \
    return NAME;                                                               \
    break;
#include <phasar/Utils/SoundinessFlags.def>
  }
}

SoundinessFlag to_SoundinessFlag(const std::string &S) {
  SoundinessFlag Type = llvm::StringSwitch<SoundinessFlag>(S)
#define SOUNDINESS_FLAG_TYPE(NAME, CMDFLAG, TYPE)                              \
  .Case(NAME, SoundinessFlag::TYPE)
#include <phasar/Utils/SoundinessFlags.def>
                            .Default(SoundinessFlag::Invalid);
  if (Type == SoundinessFlag::Invalid) {
    Type = llvm::StringSwitch<SoundinessFlag>(S)
#define SOUNDINESS_FLAG_TYPE(NAME, CMDFLAG, TYPE)                              \
  .Case(CMDFLAG, SoundinessFlag::TYPE)
#include <phasar/Utils/SoundinessFlags.def>
               .Default(SoundinessFlag::Invalid);
  }
  return Type;
}

std::ostream &operator<<(std::ostream &os, const SoundinessFlag &SF) {
  return os << to_string(SF);
}

} // namespace psr
