/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_ANALYSISSTRATEGY_STRATEGIES
#define PHASAR_PHASARLLVM_ANALYSISSTRATEGY_STRATEGIES

#include <iosfwd>
#include <string>

namespace psr {

enum class AnalysisStrategy {
#define ANALYSIS_STRATEGY_TYPES(NAME, CMDFLAG, TYPE) TYPE,
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.def"
};

std::string toString(const AnalysisStrategy &S);

AnalysisStrategy toAnalysisStrategy(const std::string &S);

std::ostream &operator<<(std::ostream &os, const AnalysisStrategy &S);

} // namespace psr

#endif
