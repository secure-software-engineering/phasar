/*
 * IFDSSummaryGenerator.hh
 *
 *  Created on: 17.05.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARYGENERATOR_HH_
#define SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARYGENERATOR_HH_

#include <set>
#include <vector>
#include <iostream>
#include "../FlowFunction.hh"
#include "../../../utils/utils.hh"
using namespace std;


enum class SummaryGenerationCTXStrategy {
	always_all = 0,
	powerset,
	all_and_none,
	all_observed,
	always_none
};

ostream& operator<<(ostream& os, const SummaryGenerationCTXStrategy& s);


template <typename N, typename D, typename M, typename I,
          typename ConcreteTabulationProblem, typename ConcreteSolver>
class IFDSSummaryGenerator {
protected:
  const M toSummarize;
  const I icfg;

  virtual vector<D> getInputs() = 0;
  virtual vector<bool> generateBitPattern(const vector<D> &inputs,
                                          const set<D> &subset) = 0;

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
  IFDSSummaryGenerator(M Function, I icfg)
      : toSummarize(Function), icfg(icfg) {}
  virtual ~IFDSSummaryGenerator() = default;
  virtual set<pair<vector<bool>, shared_ptr<FlowFunction<D>>>>
  generateSummaryFlowFunction() {
    set<pair<vector<bool>, shared_ptr<FlowFunction<D>>>> summary;
    vector<D> inputs = getInputs();
    set<D> inputset;
    inputset.insert(inputs.begin(), inputs.end());
    set<set<D>> powerset = computePowerSet(inputset);
    for (auto subset : powerset) {
      cout << "Generate summary for specific context: "
      		 << generateBitPattern(inputs, subset) << "\n";
      CTXFunctionProblem functionProblem(
          *icfg.getStartPointsOf(toSummarize).begin(), subset, icfg);
      ConcreteSolver solver(functionProblem, true);
      solver.solve();
      // get the result at the end of this function and
      // create a flow function from this set using the GenAll class
   //   summary.insert(make_pair(generateBitPattern(inputs, subset),
   //                            make_shared<GenAll<D>>(set<D>{}, 0)));
    }
    return summary;
  }
};

#endif
