/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Linus Jungemann and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_SOUNDNESSFLAG_H_
#define PHASAR_UTILS_SOUNDNESSFLAG_H_

#include <iosfwd>
#include <string>

namespace psr {

enum class SoundnessFlag {
#define SOUNDNESS_FLAG_TYPE(NAME, CMDFLAG, TYPE) TYPE,
#include <phasar/Utils/SoundnessFlag.def>
  Invalid
};

std::string to_string(const SoundnessFlag &SF);

SoundnessFlag to_SoundnessFlag(const std::string &S);

std::ostream &operator<<(std::ostream &os, const SoundnessFlag &SF);

} // namespace psr

#endif
