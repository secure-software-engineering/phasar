/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * DefaultIFDSTabulationProblem.h
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_DEFAULTIFDSTABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_IFDSIDE_DEFAULTIFDSTABULATIONPROBLEM_H_

#include <memory>

#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSTabulationProblem.h>

namespace psr {

template <typename N, typename D, typename M, typename I>
class DefaultIFDSTabulationProblem : public IFDSTabulationProblem<N, D, M, I> {
protected:
  I icfg;
  virtual D createZeroValue() = 0;
  D zerovalue;

public:
  DefaultIFDSTabulationProblem(I icfg) : icfg(icfg) {
    this->solver_config.followReturnsPastSeeds = false;
    this->solver_config.autoAddZero = true;
    this->solver_config.computeValues = true;
    this->solver_config.recordEdges = true;
    this->solver_config.computePersistedSummaries = true;
  }

  virtual ~DefaultIFDSTabulationProblem() = default;

  virtual std::shared_ptr<FlowFunction<D>>
  getSummaryFlowFunction(N callStmt, M destMthd) override {
    return nullptr;
  }

  I interproceduralCFG() override { return icfg; }

  D zeroValue() override { return zerovalue; }
};

} // namespace psr

#endif
