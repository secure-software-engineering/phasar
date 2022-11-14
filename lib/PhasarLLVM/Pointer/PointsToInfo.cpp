/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <string>

#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/Pointer/PointsToInfo.h"

std::string psr::toString(AliasResult AR) {
  switch (AR) {
  case AliasResult::NoAlias:
    return "NoAlias";
    break;
  case AliasResult::MayAlias:
    return "MayAlias";
    break;
  case AliasResult::PartialAlias:
    return "PartialAlias";
    break;
  case AliasResult::MustAlias:
    return "MustAlias";
    break;
  }
}

psr::AliasResult psr::toAliasResult(const std::string &S) {
  if (S == "NoAlias") {
    return AliasResult::NoAlias;
  }
  if (S == "MayAlias") {
    return AliasResult::MayAlias;
  }
  if (S == "PartialAlias") {
    return AliasResult::PartialAlias;
  }
  return AliasResult::MustAlias;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   const AliasResult &AR) {
  return OS << toString(AR);
}
