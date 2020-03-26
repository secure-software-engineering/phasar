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

#include "phasar/Utils/SoundnessFlag.h"

using namespace psr;

namespace psr {

std::string to_string(const SoundnessFlag &SF) {
  switch (SF) {
  default:
#define SOUNDNESS_FLAG_TYPE(NAME, CMDFLAG, TYPE)                               \
  case SoundnessFlag::TYPE:                                                    \
    return NAME;                                                               \
    break;
#include "phasar/Utils/SoundnessFlag.def"
  }
}

SoundnessFlag to_SoundnessFlag(const std::string &S) {
  SoundnessFlag Type = llvm::StringSwitch<SoundnessFlag>(S)
#define SOUNDNESS_FLAG_TYPE(NAME, CMDFLAG, TYPE)                               \
  .Case(NAME, SoundnessFlag::TYPE)
#include "phasar/Utils/SoundnessFlag.def"
                           .Default(SoundnessFlag::Invalid);
  if (Type == SoundnessFlag::Invalid) {
    Type = llvm::StringSwitch<SoundnessFlag>(S)
#define SOUNDNESS_FLAG_TYPE(NAME, CMDFLAG, TYPE)                               \
  .Case(CMDFLAG, SoundnessFlag::TYPE)
#include "phasar/Utils/SoundnessFlag.def"
               .Default(SoundnessFlag::Invalid);
  }
  return Type;
}

std::ostream &operator<<(std::ostream &os, const SoundnessFlag &SF) {
  return os << to_string(SF);
}

} // namespace psr
