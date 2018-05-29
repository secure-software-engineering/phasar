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

#ifndef SRC_ANALYSIS_IFDS_IDE_IDESUMMARYGENERATOR_H_
#define SRC_ANALYSIS_IFDS_IDE_IDESUMMARYGENERATOR_H_

#include <iostream>
#include <map>
#include <phasar/PhasarLLVM/Utils/SummaryStrategy.h>
#include <set>
#include <string>
using namespace std;
using namespace psr;
namespace psr{

template <typename N, typename D, typename M, typename I, typename V,
          typename ConcreteTabulationProblem, typename ConcreteSolver>
class IDESummaryGenerator {
protected:
  const string toSummarize;
  const I icfg;
  const SummaryGenerationStrategy CTXStrategy;

  class CTXFunctionProblem : public ConcreteTabulationProblem {
  public:
    const N start;
    set<D> facts;

    CTXFunctionProblem(N start, set<D> facts, I icfg)
        : ConcreteTabulationProblem(icfg), start(start), facts(facts) {
      this->solver_config.followReturnsPastSeeds = false;
      this->solver_config.autoAddZero = true;
      this->solver_config.computeValues = true;
      this->solver_config.recordEdges = false;
      this->solver_config.computePersistedSummaries = false;
    }

    virtual map<N, set<D>> initialSeeds() override {
      map<N, set<D>> seeds;
      seeds.insert(make_pair(start, facts));
      return seeds;
    }
  };

public:
  IDESummaryGenerator(string Function, I Icfg,
                      SummaryGenerationStrategy Strategy)
      : toSummarize(Function), icfg(Icfg), CTXStrategy(Strategy) {}
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

}//namespace psr

#endif
