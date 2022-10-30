/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/PointerAnalysisType.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"

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
  default:
#define ANALYSIS_SETUP_POINTER_TYPE(NAME, CMDFLAG, TYPE)                       \
  case PointerAnalysisType::TYPE:                                              \
    return NAME;                                                               \
    break;
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
  }
}

psr::PointerAnalysisType psr::toPointerAnalysisType(llvm::StringRef S) {
  PointerAnalysisType Type = llvm::StringSwitch<PointerAnalysisType>(S)
#define ANALYSIS_SETUP_POINTER_TYPE(NAME, CMDFLAG, TYPE)                       \
  .Case(NAME, PointerAnalysisType::TYPE)
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
                                 .Default(PointerAnalysisType::Invalid);
  if (Type == PointerAnalysisType::Invalid) {
    Type = llvm::StringSwitch<PointerAnalysisType>(S)
#define ANALYSIS_SETUP_POINTER_TYPE(NAME, CMDFLAG, TYPE)                       \
  .Case(CMDFLAG, PointerAnalysisType::TYPE)
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
               .Default(PointerAnalysisType::Invalid);
  }
  return Type;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   PointerAnalysisType PA) {
  return OS << toString(PA);
}
