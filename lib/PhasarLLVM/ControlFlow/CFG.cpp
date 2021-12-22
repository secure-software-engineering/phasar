/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/ADT/StringSwitch.h>
#include <string>

#include "phasar/PhasarLLVM/ControlFlow/CFG.h"

using namespace psr;

namespace psr {

std::string toString(const SpecialMemberFunctionType &SMFT) {
  switch (SMFT) {
  default:
#define SPECIAL_MEMBER_FUNCTION_TYPES(NAME, TYPE)                              \
  case SpecialMemberFunctionType::TYPE:                                        \
    return NAME;                                                               \
    break;
#include "phasar/PhasarLLVM/ControlFlow/SpecialMemberFunctionType.def"
  }
}

SpecialMemberFunctionType toSpecialMemberFunctionType(const std::string &SMFT) {
  SpecialMemberFunctionType Type =
      llvm::StringSwitch<SpecialMemberFunctionType>(SMFT)
#define SPECIAL_MEMBER_FUNCTION_TYPES(NAME, TYPE)                              \
  .Case(NAME, SpecialMemberFunctionType::TYPE)
#include "phasar/PhasarLLVM/ControlFlow/SpecialMemberFunctionType.def"
          .Default(SpecialMemberFunctionType::None);
  return Type;
}

std::ostream &operator<<(std::ostream &OS,
                         const SpecialMemberFunctionType &SMFT) {
  return OS << toString(SMFT);
}

} // namespace psr
