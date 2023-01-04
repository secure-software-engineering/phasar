/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/Pointer/PointerAnalysisType.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"

std::string psr::toString(AliasResult AR) {
  switch (AR) {
  case AliasResult::NoAlias:
    return "NoAlias";
  case AliasResult::MayAlias:
    return "MayAlias";
  case AliasResult::PartialAlias:
    return "PartialAlias";
  case AliasResult::MustAlias:
    return "MustAlias";
  }
}

psr::AliasResult psr::toAliasResult(llvm::StringRef S) {
  return llvm::StringSwitch<AliasResult>(S)
      .Case("NoAlias", AliasResult::NoAlias)
      .Case("MayAlias", AliasResult::MayAlias)
      .Case("PartialAlias", AliasResult::PartialAlias)
      .Default(AliasResult::MustAlias);
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS, AliasResult AR) {
  return OS << toString(AR);
}

std::string psr::toString(PointerAnalysisType PA) {
  switch (PA) {
#define POINTER_ANALYSIS_TYPE(NAME, CMDFLAG, TYPE)                             \
  case PointerAnalysisType::NAME:                                              \
    return #NAME;
#include "phasar/Pointer/PointerAnalysisType.def"
  case PointerAnalysisType::Invalid:
    return "Invalid";
  }
}

psr::PointerAnalysisType psr::toPointerAnalysisType(llvm::StringRef S) {
  PointerAnalysisType Type = llvm::StringSwitch<PointerAnalysisType>(S)
#define POINTER_ANALYSIS_TYPE(NAME, CMDFLAG, TYPE)                             \
  .Case(#NAME, PointerAnalysisType::NAME)
#include "phasar/Pointer/PointerAnalysisType.def"
                                 .Default(PointerAnalysisType::Invalid);
  if (Type == PointerAnalysisType::Invalid) {
    Type = llvm::StringSwitch<PointerAnalysisType>(S)
#define POINTER_ANALYSIS_TYPE(NAME, CMDFLAG, TYPE)                             \
  .Case(CMDFLAG, PointerAnalysisType::NAME)
#include "phasar/Pointer/PointerAnalysisType.def"
               .Default(PointerAnalysisType::Invalid);
  }
  return Type;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   PointerAnalysisType PA) {
  return OS << toString(PA);
}
