/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_SOUNDINESSFLAGS_H_
#define PHASAR_UTILS_SOUNDINESSFLAGS_H_

namespace psr {

enum class SoundinessFlag {
#define SOUNDINESS_FLAG_TYPE(NAME, CMDFLAG, TYPE) TYPE,
#include <phasar/Utils/SoundinessFlags.def>
  Invalid
};

std::string to_string(const SoundinessFlag &SF);

SoundinessFlag to_SoundinessFlag(const std::string &S);

std::ostream &operator<<(std::ostream &os, const SoundinessFlag &SF);

} // namespace psr

#endif
