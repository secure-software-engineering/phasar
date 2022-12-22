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

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {

enum class Soundness {
#define SOUNDNESS_FLAG_TYPE(NAME, CMDFLAG, DESC) NAME,
#include "phasar/Utils/Soundness.def"
  Invalid
};

std::string toString(Soundness S);

Soundness toSoundness(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Soundness S);

} // namespace psr

#endif
