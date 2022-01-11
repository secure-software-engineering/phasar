/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Linus Jungemann and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_SOUNDNESS_H_
#define PHASAR_UTILS_SOUNDNESS_H_

#include <iosfwd>
#include <string>

namespace psr {

enum class Soundness {
#define SOUNDNESS_FLAG_TYPE(NAME, TYPE) TYPE,
#include "phasar/Utils/Soundness.def"
  Invalid
};

std::string toString(const Soundness &S);

Soundness toSoundness(const std::string &S);

std::ostream &operator<<(std::ostream &OS, const Soundness &S);

} // namespace psr

#endif
