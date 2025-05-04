/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/Pointer/AliasResult.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

std::string psr::toString(AliasResult AR) {
  switch (AR) {
#define ALIAS_RESULT_TYPE(NAME)                                                \
  case AliasResult::NAME:                                                      \
    return #NAME;
#include "phasar/Pointer/AliasResult.def"
  }
  llvm_unreachable(
      "All AliasResult alternatives should be handled in the switch above");
}

psr::AliasResult psr::toAliasResult(llvm::StringRef S) {
  return llvm::StringSwitch<AliasResult>(S)
#define ALIAS_RESULT_TYPE(NAME) .Case(#NAME, AliasResult::NAME)
      .Default(AliasResult::MayAlias); // Sound overapproximation
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS, AliasResult AR) {
  return OS << toString(AR);
}
