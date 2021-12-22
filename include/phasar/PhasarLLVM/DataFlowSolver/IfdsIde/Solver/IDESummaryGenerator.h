/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSSummaryGenerator.h
 *
 *  Created on: 24.05.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_IDESUMMARYGENERATOR_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_IDESUMMARYGENERATOR_H

#include <map>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/Utils/SummaryStrategy.h"

namespace psr {

template <typename N, typename D, typename F, typename I, typename V,
          typename ConcreteTabulationProblem, typename ConcreteSolver>
class IDESummaryGenerator {
protected:
  const std::string ToSummarize;
  const I ICFG;
  const SummaryGenerationStrategy CTXStrategy;

  class CTXFunctionProblem : public ConcreteTabulationProblem {
  public:
    const N Start;
    std::set<D> Facts;

    CTXFunctionProblem(N Start, std::set<D> Facts, I ICFG)
        : ConcreteTabulationProblem(ICFG), Start(Start), Facts(Facts) {
      this->solver_config.followReturnsPastSeeds = false;
      this->solver_config.autoAddZero = true;
      this->solver_config.computeValues = true;
      this->solver_config.recordEdges = false;
      this->solver_config.computePersistedSummaries = false;
    }

    std::map<N, std::set<D>> initialSeeds() override {
      std::map<N, std::set<D>> Seeds;
      Seeds.insert({Start, Facts});
      return Seeds;
    }
  };

public:
  IDESummaryGenerator(std::string Function, I Icfg,
                      SummaryGenerationStrategy Strategy)
      : ToSummarize(std::move(Function)), ICFG(Icfg), CTXStrategy(Strategy) {}
  virtual ~IDESummaryGenerator() = default;
  void generateSummaries() {
    // initialize the input combinations that should be considered
    switch (CTXStrategy) {
    case SummaryGenerationStrategy::always_all:

      break;
    case SummaryGenerationStrategy::always_none:

      break;
    case SummaryGenerationStrategy::all_and_none:

      break;
    case SummaryGenerationStrategy::powerset:

      break;
    case SummaryGenerationStrategy::all_observed:
      // TODO here we have to track what we have already observed first!
      break;
    }
  }
};

} // namespace psr

#endif
