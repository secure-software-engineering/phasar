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

#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.h"

namespace psr {

std::string toString(const AnalysisStrategy &S) {
  switch (S) {
  default:
#define ANALYSIS_STRATEGY_TYPES(NAME, CMDFLAG, TYPE)                           \
  case AnalysisStrategy::TYPE:                                                 \
    return NAME;                                                               \
    break;
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.def"
  }
}

AnalysisStrategy toAnalysisStrategy(const std::string &S) {
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

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const AnalysisStrategy &S) {
  return OS << toString(S);
}

} // namespace psr
