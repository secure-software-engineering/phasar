/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Utils/SummaryStrategy.h"

#include "llvm/Support/raw_ostream.h"

namespace psr {

const std::map<SummaryGenerationStrategy, std::string>
    SummaryGenerationStrategyToString = {
        {SummaryGenerationStrategy::always_all, "always_all"},
        {SummaryGenerationStrategy::always_none, "always_none"},
        {SummaryGenerationStrategy::all_and_none, "all_and_none"},
        {SummaryGenerationStrategy::powerset, "powerset"},
        {SummaryGenerationStrategy::all_observed, "all_observed"}};

const std::map<std::string, SummaryGenerationStrategy>
    StringToSummaryGenerationStrategy = {
        {"always_all", SummaryGenerationStrategy::always_all},
        {"always_none", SummaryGenerationStrategy::always_none},
        {"all_and_none", SummaryGenerationStrategy::all_and_none},
        {"powerset", SummaryGenerationStrategy::powerset},
        {"all_observed", SummaryGenerationStrategy::all_observed}

};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const SummaryGenerationStrategy &S) {
  return OS << SummaryGenerationStrategyToString.at(S);
}
} // namespace psr
