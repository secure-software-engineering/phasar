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
 *  Created on: 17.05.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_IFDSSUMMARYGENERATOR_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_IFDSSUMMARYGENERATOR_H_

#include <iostream> // std::cout
#include <set>
#include <vector>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Utils/SummaryStrategy.h"

namespace psr {

template <typename N, typename D, typename F, typename I,
          typename ConcreteTabulationProblem, typename ConcreteSolver>
class IFDSSummaryGenerator {
protected:
  const F toSummarize;
  const I icfg;
  const SummaryGenerationStrategy CTXStrategy;

  virtual std::vector<D> getInputs() = 0;
  virtual std::vector<bool> generateBitPattern(const std::vector<D> &inputs,
                                               const std::set<D> &subset) = 0;

  class CTXFunctionProblem : public ConcreteTabulationProblem {
  public:
    const N start;
    std::set<D> facts;

    CTXFunctionProblem(N start, std::set<D> facts, I icfg)
        : ConcreteTabulationProblem(icfg), start(start), facts(facts) {
      this->solver_config.followReturnsPastSeeds = false;
      this->solver_config.autoAddZero = true;
      this->solver_config.computeValues = true;
      this->solver_config.recordEdges = false;
      this->solver_config.computePersistedSummaries = false;
    }

    virtual std::map<N, std::set<D>> initialSeeds() override {
      std::map<N, std::set<D>> seeds;
      seeds.insert(make_pair(start, facts));
      return seeds;
    }
  };

public:
  IFDSSummaryGenerator(F Function, I icfg, SummaryGenerationStrategy Strategy)
      : toSummarize(Function), icfg(icfg), CTXStrategy(Strategy) {}
  virtual ~IFDSSummaryGenerator() = default;
  virtual std::set<
      std::pair<std::vector<bool>, std::shared_ptr<FlowFunction<D>>>>
  generateSummaryFlowFunction() {
    std::set<std::pair<std::vector<bool>, std::shared_ptr<FlowFunction<D>>>>
        summary;
    std::vector<D> inputs = getInputs();
    std::set<D> inputset;
    inputset.insert(inputs.begin(), inputs.end());
    std::set<std::set<D>> InputCombinations;
    // initialize the input combinations that should be considered
    switch (CTXStrategy) {
    case SummaryGenerationStrategy::always_all:
      InputCombinations.insert(inputset);
      break;
    case SummaryGenerationStrategy::always_none:
      InputCombinations.insert(std::set<D>());
      break;
    case SummaryGenerationStrategy::all_and_none:
      InputCombinations.insert(inputset);
      InputCombinations.insert(std::set<D>());
      break;
    case SummaryGenerationStrategy::powerset:
      InputCombinations = computePowerSet(inputset);
      break;
    case SummaryGenerationStrategy::all_observed:
      // TODO here we have to track what we have already observed first!
      break;
    }
    for (auto subset : InputCombinations) {
      std::cout << "Generate summary for specific context: "
                << generateBitPattern(inputs, subset) << "\n";
      CTXFunctionProblem functionProblem(
          *icfg.getStartPointsOf(toSummarize).begin(), subset, icfg);
      ConcreteSolver solver(functionProblem, true);
      solver.solve();
      // get the result at the end of this function and
      // create a flow function from this set using the GenAll class
      std::set<N> LastInsts = icfg.getExitPointsOf(toSummarize);
      std::set<D> results;
      for (auto fact : solver.resultsAt(*LastInsts.begin())) {
        results.insert(fact.first);
      }
      summary.insert(make_pair(
          generateBitPattern(inputs, subset),
          std::make_shared<GenAll<D>>(results, LLVMZeroValue::getInstance())));
    }
    return summary;
  }
};

} // namespace psr

#endif
