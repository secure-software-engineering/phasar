/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_ANALYSISSTRATEGY_STRATEGIES_H
#define PHASAR_ANALYSISSTRATEGY_STRATEGIES_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {

enum class AnalysisStrategy {
  None,
#define ANALYSIS_STRATEGY_TYPES(NAME, CMDFLAG, DESC) NAME,
#include "phasar/AnalysisStrategy/Strategies.def"

};

std::string toString(const AnalysisStrategy &S);

AnalysisStrategy toAnalysisStrategy(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, AnalysisStrategy S);

} // namespace psr

#endif
