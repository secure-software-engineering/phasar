/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_ANALYSISSTRATEGY_STRATEGIES_H
#define PHASAR_PHASARLLVM_ANALYSISSTRATEGY_STRATEGIES_H

#include "llvm/Support/raw_ostream.h"
#include <string>

namespace psr {

enum class AnalysisStrategy {
#define ANALYSIS_STRATEGY_TYPES(NAME, CMDFLAG, TYPE) TYPE,
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.def"
};

std::string toString(const AnalysisStrategy &S);

AnalysisStrategy toAnalysisStrategy(const std::string &S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const AnalysisStrategy &S);

} // namespace psr

#endif
