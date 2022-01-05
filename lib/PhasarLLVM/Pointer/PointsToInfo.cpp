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

#include "phasar/PhasarLLVM/Pointer/PointsToInfo.h"

namespace psr {

std::string toString(AliasResult AR) {
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

AliasResult toAliasResult(const std::string &S) {
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

std::ostream &operator<<(std::ostream &OS, const AliasResult &AR) {
  return OS << toString(AR);
}

std::string tostring(const PointerAnalysisType &PA) {
  switch (PA) {
  default:
#define ANALYSIS_SETUP_POINTER_TYPE(NAME, CMDFLAG, TYPE)                       \
  case PointerAnalysisType::TYPE:                                              \
    return NAME;                                                               \
    break;
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
  }
}

PointerAnalysisType toPointerAnalysisType(const std::string &S) {
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

std::ostream &operator<<(std::ostream &OS, const PointerAnalysisType &PA) {
  return OS << tostring(PA);
}

} // namespace psr
