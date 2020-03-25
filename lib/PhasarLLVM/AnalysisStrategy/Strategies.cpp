/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <ostream>
#include <string>

#include "llvm/ADT/StringSwitch.h"

#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.h"

using namespace psr;

namespace psr {

std::string to_string(const AnalysisStrategy &S) {
  switch (S) {
  default:
#define ANALYSIS_STRATEGY_TYPES(NAME, CMDFLAG, TYPE)                           \
  case AnalysisStrategy::TYPE:                                                 \
    return NAME;                                                               \
    break;
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.def"
  }
}

AnalysisStrategy to_AnalysisStrategy(const std::string &S) {
  AnalysisStrategy Type = llvm::StringSwitch<AnalysisStrategy>(S)
#define ANALYSIS_STRATEGY_TYPES(NAME, CMDFLAG, TYPE)                           \
  .Case(NAME, AnalysisStrategy::TYPE)
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.def"
                              .Default(AnalysisStrategy::None);
  if (Type == AnalysisStrategy::None) {
    Type = llvm::StringSwitch<AnalysisStrategy>(S)
#define ANALYSIS_STRATEGY_TYPES(NAME, CMDFLAG, TYPE)                           \
  .Case(CMDFLAG, AnalysisStrategy::TYPE)
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.def"
               .Default(AnalysisStrategy::None);
  }
  return Type;
}

std::ostream &operator<<(std::ostream &os, const AnalysisStrategy &S) {
  return os << to_string(S);
}

} // namespace psr
