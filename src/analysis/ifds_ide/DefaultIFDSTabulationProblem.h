/*
 * DefaultIFDSTabulationProblem.h
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_DEFAULTIFDSTABULATIONPROBLEM_H_
#define ANALYSIS_IFDS_IDE_DEFAULTIFDSTABULATIONPROBLEM_H_

#include "FlowFunctions.h"
#include "IFDSTabulationProblem.h"
#include "flow_func/Identity.h"
#include <type_traits>

template <class N, class D, class M, class I>
class DefaultIFDSTabulationProblem : public IFDSTabulationProblem<N, D, M, I> {
protected:
  I icfg;
  virtual D createZeroValue() = 0;
  D zerovalue;

public:
  DefaultIFDSTabulationProblem(I icfg) : icfg(icfg) {
    // set to the default solver configuration
    this->solver_config.followReturnsPastSeeds = false;
    this->solver_config.autoAddZero = true;
    this->solver_config.computeValues = true;
    this->solver_config.recordEdges = true;
    this->solver_config.computePersistedSummaries = true;
  }

  virtual ~DefaultIFDSTabulationProblem() = default;

  virtual shared_ptr<FlowFunction<D>>
  getSummaryFlowFunction(N callStmt, M destMthd) override {
    return nullptr;
  }

  I interproceduralCFG() override { return icfg; }

  D zeroValue() override { return zerovalue; }
};

#endif /* ANALYSIS_IFDS_IDE_DEFAULTIFDSTABULATIONPROBLEM_HH_ */
