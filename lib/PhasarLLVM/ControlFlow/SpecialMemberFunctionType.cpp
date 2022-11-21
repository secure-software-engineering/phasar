/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/SpecialMemberFunctionType.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"

using namespace psr;

std::string psr::toString(SpecialMemberFunctionType SMFT) {
  switch (SMFT) {
  default:
#define SPECIAL_MEMBER_FUNCTION_TYPES(NAME, TYPE)                              \
  case SpecialMemberFunctionType::TYPE:                                        \
    return NAME;                                                               \
    break;
#include "phasar/PhasarLLVM/ControlFlow/SpecialMemberFunctionType.def"
  }
}

SpecialMemberFunctionType
psr::toSpecialMemberFunctionType(llvm::StringRef SMFT) {
  SpecialMemberFunctionType Type =
      llvm::StringSwitch<SpecialMemberFunctionType>(SMFT)
#define SPECIAL_MEMBER_FUNCTION_TYPES(NAME, TYPE)                              \
  .Case(NAME, SpecialMemberFunctionType::TYPE)
#include "phasar/PhasarLLVM/ControlFlow/SpecialMemberFunctionType.def"
          .Default(SpecialMemberFunctionType::None);
  return Type;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   SpecialMemberFunctionType SMFT) {
  return OS << toString(SMFT);
}
